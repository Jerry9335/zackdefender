#include "VirusTotalEngine.h"
#include <QCoreApplication>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QCryptographicHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QTimer>
#include <QUrl>

VirusTotalEngine::VirusTotalEngine(QObject *parent)
    : ScanEngine(QStringLiteral("VirusTotal"), parent)
    , m_nam(new QNetworkAccessManager(this))
{
    // Rate limiter: max 4 req/min for free tier, 1 req/500ms for safety
    m_rateTimer = new QTimer(this);
    m_rateTimer->setSingleShot(true);
    m_rateTimer->setInterval(200); // ~5 req/sec with concurrency=2

    connect(m_nam, &QNetworkAccessManager::finished,
            this, &VirusTotalEngine::onReplyFinished);
}

void VirusTotalEngine::start(ScanType scanType, const QString &customPath)
{
    if (m_scanning) return;
    if (m_apiKey.isEmpty()) {
        emit scanOutput(m_engineName,
            QStringLiteral("⚠ VirusTotal 引擎未配置 API 密钥，跳过"));
        emit finished(m_engineName, -2, 0);
        return;
    }

    m_scanning = true;
    m_progress = 0;
    m_cancelled = false;
    m_scannedFiles = 0;
    m_threatsFound = 0;
    m_inFlight = 0;
    m_pendingFiles.clear();
    m_queried.clear();
    emit scanningChanged();
    emit progressChanged();
    emit scanOutput(m_engineName, QStringLiteral("☁️ VirusTotal 云引擎开始扫描..."));

    m_currentFile = QStringLiteral("正在收集文件列表...");
    emit currentFileChanged();

    // Collect targets (same logic as HashEngine)
    QStringList targets;
    switch (scanType) {
    case QuickScan:
        // High-value targets: user dirs + temp + startup
        targets << QDir::tempPath();
        targets << QDir::homePath() + "/Downloads";
        targets << QDir::homePath() + "/Desktop";
        targets << QDir::homePath() + "/AppData/Roaming/Microsoft/Windows/Start Menu/Programs/Startup";
        targets << QDir::homePath() + "/AppData/Local/Temp";
        break;
    case FullScan:
        foreach (const QFileInfo &drive, QDir::drives())
            targets << drive.absoluteFilePath();
        break;
    case CustomScan:
        if (!customPath.isEmpty()) targets << customPath;
        break;
    }

    // Only scan executable-like files + archives for VT (API calls are expensive)
    QStringList exts = { "*.exe", "*.dll", "*.scr", "*.msi", "*.com", "*.pif", "*.sys",
                         "*.zip", "*.7z", "*.rar" };
    for (const QString &target : targets) {
        QDirIterator it(target, exts, QDir::Files,
                        scanType == FullScan
                            ? QDirIterator::Subdirectories | QDirIterator::FollowSymlinks
                            : QDirIterator::Subdirectories);
        while (it.hasNext()) {
            QString path = it.next();
            if (!isExcluded(path))
                m_pendingFiles << path;
        }
    }

    m_totalFiles = m_pendingFiles.size();
    emit scanOutput(m_engineName,
        QString("📁 VirusTotal 找到 %1 个可查询文件 (限速: ~1.6次/秒)").arg(m_totalFiles));

    // Start processing
    QTimer::singleShot(100, this, &VirusTotalEngine::processNext);
}

void VirusTotalEngine::processNext()
{
    if (m_cancelled) {
        m_scanning = false;
        emit scanningChanged();
        emit finished(m_engineName, -1, m_threatsFound);
        return;
    }

    // Process up to 2 files concurrently (rate limited)
    if (!m_pendingFiles.isEmpty() && m_inFlight < 2) {
        QString filePath = m_pendingFiles.takeFirst();

        // Compute SHA256
        QString sha256 = computeSha256(filePath);
        if (sha256.isEmpty() || m_queried.contains(sha256)) {
            // Skip unreadable or already-queried
            m_scannedFiles++;
            m_progress = m_totalFiles > 0 ? (m_scannedFiles * 100 / m_totalFiles) : 0;
            emit progressChanged();
            QTimer::singleShot(10, this, &VirusTotalEngine::processNext);
            return;
        }

        m_queried.insert(sha256);
        queryHash(filePath, sha256);
        m_inFlight++;
    }

    if (m_pendingFiles.isEmpty() && m_inFlight == 0) {
        // All done
        m_scanning = false;
        m_progress = 100;
        emit progressChanged();
        emit scanningChanged();
        emit scanOutput(m_engineName,
            QString("✅ VirusTotal 完成 — 查询 %1 个文件, 发现 %2 个威胁")
                .arg(m_scannedFiles).arg(m_threatsFound));
        emit finished(m_engineName, 0, m_threatsFound);
    } else if (!m_pendingFiles.isEmpty()) {
        // Schedule next batch after rate limit
        m_rateTimer->start();
        connect(m_rateTimer, &QTimer::timeout, this, &VirusTotalEngine::processNext,
                Qt::SingleShotConnection);
    }
}

void VirusTotalEngine::queryHash(const QString &filePath, const QString &sha256)
{
    QUrl url(QString("https://www.virustotal.com/api/v3/files/%1").arg(sha256));
    QNetworkRequest req(url);
    req.setRawHeader("x-apikey", m_apiKey.toUtf8());
    req.setRawHeader("Accept", "application/json");

    m_currentFile = filePath;
    emit currentFileChanged();

    QNetworkReply *reply = m_nam->get(req);
    // Bind file path to this specific reply — eliminates race condition
    // when multiple requests are in flight concurrently
    reply->setProperty("filePath", filePath);
}

void VirusTotalEngine::onReplyFinished(QNetworkReply *reply)
{
    m_inFlight--;
    reply->deleteLater();

    if (m_cancelled) return;

    // Retrieve the correct file path from THIS reply (not shared m_currentFile!)
    QString filePath = reply->property("filePath").toString();

    int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    QByteArray data = reply->readAll();

    if (status == 200) {
        QJsonDocument doc = QJsonDocument::fromJson(data);
        QJsonObject obj = doc.object();
        QJsonObject dataObj = obj["data"].toObject();
        QJsonObject attrs = dataObj["attributes"].toObject();
        QJsonObject stats = attrs["last_analysis_stats"].toObject();

        int malicious = stats["malicious"].toInt();
        int suspicious = stats["suspicious"].toInt();
        int total = stats["harmless"].toInt() + stats["malicious"].toInt()
                  + stats["suspicious"].toInt() + stats["undetected"].toInt()
                  + stats["timeout"].toInt();

        if (malicious > 0 || suspicious > 0) {
            m_threatsFound++;
            QString desc = QString("VirusTotal: %1/%2 引擎检测到威胁 (恶意:%3 可疑:%4)")
                .arg(malicious + suspicious).arg(total).arg(malicious).arg(suspicious);
            emit threatFound(m_engineName, filePath, desc,
                           qMin(5, malicious > 0 ? malicious + 1 : 3));
            emit scanOutput(m_engineName,
                QString("⚠ VT命中 [%1/%2]: %3")
                    .arg(malicious + suspicious).arg(total).arg(filePath));
        } else {
            emit scanOutput(m_engineName,
                QString("✓ VT干净 [0/%1]: %2").arg(total).arg(filePath));
        }
    } else if (status == 404) {
        // Hash not found in VT — file never scanned, skip
        emit scanOutput(m_engineName,
            QString("○ VT未见: %1").arg(filePath));
    } else if (status == 429) {
        // Rate limited — slow down
        emit scanOutput(m_engineName, QStringLiteral("⏳ VT限速，自动降速..."));
        m_rateTimer->setInterval(m_rateTimer->interval() + 500);
        // Re-queue this file
        m_inFlight = 0;
        // We lost the file, just continue
    } else if (status == 401 || status == 403) {
        emit scanOutput(m_engineName,
            QStringLiteral("⚠ VT API密钥无效或权限不足，停止查询"));
        m_pendingFiles.clear();
    } else {
        emit scanOutput(m_engineName,
            QString("⚠ VT错误 [HTTP %1]: %2").arg(status).arg(filePath));
    }

    m_scannedFiles++;
    m_progress = m_totalFiles > 0 ? qMin(99, m_scannedFiles * 100 / m_totalFiles) : 0;
    emit progressChanged();

    // Continue processing
    QTimer::singleShot(50, this, &VirusTotalEngine::processNext);
}

void VirusTotalEngine::cancel()
{
    m_cancelled = true;
    m_pendingFiles.clear();
}

QString VirusTotalEngine::computeSha256(const QString &filePath) const
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) return {};

    QCryptographicHash hash(QCryptographicHash::Sha256);
    // Read up to 32MB (VT free tier limit)
    QByteArray chunk;
    qint64 totalRead = 0;
    const qint64 maxRead = 32 * 1024 * 1024;
    while (totalRead < maxRead && !file.atEnd()) {
        chunk = file.read(65536);
        if (chunk.isEmpty()) break;
        hash.addData(chunk);
        totalRead += chunk.size();
    }
    file.close();

    return QString::fromUtf8(hash.result().toHex());
}

bool VirusTotalEngine::isExcluded(const QString &path) const
{
    // Skip recycle bin
    QString lower = path.toLower();
    if (lower.contains("/$recycle.bin/") || lower.contains("\\$recycle.bin\\"))
        return true;

    // ── Self-protection: skip our own program directory ──
    QString appDir = QCoreApplication::applicationDirPath();
    if (path.startsWith(appDir, Qt::CaseInsensitive))
        return true;
    QString deployDir = QDir::cleanPath(appDir + "/../deploy");
    QString buildDir = QDir::cleanPath(appDir + "/../build");
    if (path.startsWith(deployDir, Qt::CaseInsensitive))
        return true;
    if (path.startsWith(buildDir, Qt::CaseInsensitive))
        return true;

    QFileInfo fi(path);
    if (fi.size() < 512 || fi.size() > 200 * 1024 * 1024)
        return true;

    return false;
}

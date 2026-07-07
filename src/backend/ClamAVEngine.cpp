#include "ClamAVEngine.h"
#include <QCoreApplication>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSettings>
#include <QTemporaryFile>
#include <QTextStream>
#include <QTimer>

static const char *SETTINGS_ORG = "ZackDefender";
static const char *SETTINGS_APP = "ZackDefender";

// ── File type category definitions ──────────────────────────────────
static const QStringList kExecutables = {"*.exe","*.dll","*.com","*.scr","*.sys","*.drv"};
static const QStringList kScripts     = {"*.bat","*.cmd","*.ps1","*.vbs","*.js","*.wsf","*.wsh","*.hta"};
static const QStringList kInstallers  = {"*.msi","*.pif","*.cpl","*.jar"};
static const QStringList kArchives    = {"*.zip","*.7z","*.rar","*.tar","*.gz","*.bz2"};
static const QStringList kDocuments   = {"*.docm","*.xlsm","*.pptm","*.pdf","*.rtf"};

template<typename T>
T ClamAVEngine::readSetting(const QString &key, const T &defaultVal) const
{
    QSettings s(SETTINGS_ORG, SETTINGS_APP);
    return s.value(key, defaultVal).template value<T>();
}

// ── Category getters/setters ────────────────────────────────────────
bool ClamAVEngine::scanExecutables() const { return readSetting("ClamAV/scanExecutables", true); }
void ClamAVEngine::setScanExecutables(bool v) {
    QSettings s(SETTINGS_ORG, SETTINGS_APP);
    s.setValue("ClamAV/scanExecutables", v);
    emit fileTypesChanged();
}
bool ClamAVEngine::scanScripts() const { return readSetting("ClamAV/scanScripts", true); }
void ClamAVEngine::setScanScripts(bool v) {
    QSettings s(SETTINGS_ORG, SETTINGS_APP);
    s.setValue("ClamAV/scanScripts", v);
    emit fileTypesChanged();
}
bool ClamAVEngine::scanInstallers() const { return readSetting("ClamAV/scanInstallers", true); }
void ClamAVEngine::setScanInstallers(bool v) {
    QSettings s(SETTINGS_ORG, SETTINGS_APP);
    s.setValue("ClamAV/scanInstallers", v);
    emit fileTypesChanged();
}
bool ClamAVEngine::scanArchives() const { return readSetting("ClamAV/scanArchives", true); }
void ClamAVEngine::setScanArchives(bool v) {
    QSettings s(SETTINGS_ORG, SETTINGS_APP);
    s.setValue("ClamAV/scanArchives", v);
    emit fileTypesChanged();
}
bool ClamAVEngine::scanDocuments() const { return readSetting("ClamAV/scanDocuments", false); }
void ClamAVEngine::setScanDocuments(bool v) {
    QSettings s(SETTINGS_ORG, SETTINGS_APP);
    s.setValue("ClamAV/scanDocuments", v);
    emit fileTypesChanged();
}

// ── Build active extension list from settings ───────────────────────
QStringList ClamAVEngine::activeExtensions() const
{
    QStringList result;
    if (scanExecutables()) result << kExecutables;
    if (scanScripts())     result << kScripts;
    if (scanInstallers())  result << kInstallers;
    if (scanArchives())    result << kArchives;
    if (scanDocuments())   result << kDocuments;
    return result;
}

ClamAVEngine::ClamAVEngine(QObject *parent)
    : ScanEngine(QStringLiteral("ClamAV"), parent)
{
    resolveEnginePath();
}

// ── Path resolution ────────────────────────────────────────────────
void ClamAVEngine::resolveEnginePath()
{
    QStringList candidates;
    QString appDir = QCoreApplication::applicationDirPath();
    candidates << appDir + "/engines/clamav/clamscan.exe";
    candidates << QDir::cleanPath(appDir + "/../deploy/engines/clamav/clamscan.exe");
    candidates << "clamscan.exe";

    for (const QString &candidate : candidates) {
        if (candidate.contains('/') || candidate.contains('\\')) {
            if (QFileInfo::exists(candidate)) {
                m_enginePath = QDir::toNativeSeparators(QFileInfo(candidate).absoluteFilePath());
                m_available = true;
                return;
            }
        }
    }

    QProcess where;
    where.start("where", QStringList() << "clamscan.exe");
    where.waitForFinished(3000);
    if (where.exitCode() == 0) {
        QString output = QString::fromLocal8Bit(where.readAllStandardOutput()).trimmed();
        QStringList lines = output.split('\n', Qt::SkipEmptyParts);
        if (!lines.isEmpty()) {
            m_enginePath = lines.first().trimmed();
            m_available = QFileInfo::exists(m_enginePath);
            if (m_available) return;
        }
    }
    m_available = false;
    m_enginePath.clear();
}

// ── Start scan ────────────────────────────────────────────────────
void ClamAVEngine::start(ScanType scanType, const QString &customPath)
{
    if (m_scanning) return;
    if (!m_available) {
        emit scanOutput(m_engineName,
            QStringLiteral("⚠ ClamAV 引擎不可用 — 未找到 clamscan.exe"));
        emit finished(m_engineName, -2, 0);
        return;
    }

    m_scanning = true;
    m_progress = 0;
    m_cancelled = false;
    m_threatsFound = 0;
    m_scannedCount = 0;
    m_totalFiles = 0;
    m_buffer.clear();
    emit scanningChanged();
    emit progressChanged();
    emit scanOutput(m_engineName, QStringLiteral("🦀 ClamAV 引擎开始扫描..."));

    QStringList targets = collectTargets(scanType, customPath);
    if (targets.isEmpty()) {
        emit scanOutput(m_engineName, QStringLiteral("⚠ 没有可扫描的目标"));
        m_scanning = false; emit scanningChanged();
        emit finished(m_engineName, 0, 0);
        return;
    }

    // ── Get active extensions from settings ──────────────────
    QStringList exts = activeExtensions();
    if (exts.isEmpty()) {
        emit scanOutput(m_engineName, QStringLiteral("⚠ 未选择任何文件类型，请在设置中勾选"));
        m_scanning = false; emit scanningChanged();
        emit finished(m_engineName, 0, 0);
        return;
    }

    QStringList extNames;
    if (scanExecutables()) extNames << "可执行文件";
    if (scanScripts())     extNames << "脚本";
    if (scanInstallers())  extNames << "安装包";
    if (scanArchives())    extNames << "压缩包";
    if (scanDocuments())   extNames << "文档";
    emit scanOutput(m_engineName,
        QString("📋 扫描类型: %1").arg(extNames.join(", ")));

    // ── Collect files ────────────────────────────────────────
    emit scanOutput(m_engineName,
        QString("📊 正在收集文件 (%1 个目录)...").arg(targets.size()));
    m_currentFile = QStringLiteral("正在收集文件列表...");
    emit currentFileChanged();
    QCoreApplication::processEvents();

    QStringList fileList;
    int dirIdx = 0;
    for (const QString &target : targets) {
        dirIdx++;
        if (dirIdx % 2 == 0) {
            m_currentFile = QString("收集: %1/%2 目录 (%3 个文件)...")
                .arg(dirIdx).arg(targets.size()).arg(fileList.size());
            emit currentFileChanged();
            QCoreApplication::processEvents();
        }

        for (const QString &pattern : exts) {
            QDirIterator it(target, {pattern}, QDir::Files,
                            QDirIterator::Subdirectories | QDirIterator::FollowSymlinks);
            while (it.hasNext()) {
                fileList << it.next();
                if (fileList.size() % 500 == 0) {
                    m_currentFile = QString("已收集 %1 个文件...").arg(fileList.size());
                    emit currentFileChanged();
                    QCoreApplication::processEvents();
                }
            }
        }
    }

    m_totalFiles = fileList.size();
    emit scanOutput(m_engineName,
        QString("📊 收集完成: %1 个文件").arg(m_totalFiles));
    m_currentFile = QString("共 %1 个文件，启动扫描...").arg(m_totalFiles);
    emit currentFileChanged();
    QCoreApplication::processEvents();

    if (m_totalFiles == 0) {
        emit scanOutput(m_engineName, QStringLiteral("✅ 没有匹配的文件"));
        m_scanning = false; m_progress = 100;
        emit progressChanged(); emit scanningChanged();
        emit finished(m_engineName, 0, 0);
        return;
    }

    // ── Write file list to temp ──────────────────────────────
    m_fileListTemp = new QTemporaryFile(this);
    m_fileListTemp->setFileTemplate(QDir::tempPath() + "/zd_clamav_XXXXXX.txt");
    if (!m_fileListTemp->open()) {
        emit scanOutput(m_engineName, QStringLiteral("❌ 无法创建临时文件"));
        m_scanning = false; emit scanningChanged();
        emit finished(m_engineName, -4, 0);
        return;
    }

    {
        QTextStream ts(m_fileListTemp);
        for (const QString &f : fileList)
            ts << QDir::toNativeSeparators(f) << "\n";
        m_fileListTemp->flush();
    }

    // ── Start clamscan ───────────────────────────────────────
    QStringList args = buildArgs(scanType, customPath);
    args << "--file-list=" + m_fileListTemp->fileName();

    emit scanOutput(m_engineName,
        QString("⚙ 启动扫描: %1 个文件...").arg(fileList.size()));

    m_process = new QProcess(this);
    m_process->setProcessChannelMode(QProcess::MergedChannels);
    connect(m_process, &QProcess::readyRead, this, &ClamAVEngine::onReadyRead);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &ClamAVEngine::onProcessFinished);
    connect(m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError) {
        emit scanOutput(m_engineName,
            QString("❌ 进程错误: %1").arg(m_process ? m_process->errorString() : "unknown"));
    });

    m_currentFile = QStringLiteral("正在启动 ClamAV...");
    emit currentFileChanged();
    m_progress = 1;
    emit progressChanged();

    m_process->start(m_enginePath, args);
}

// ── Build clamscan arguments ──────────────────────────────────────
QStringList ClamAVEngine::buildArgs(ScanType scanType, const QString &) const
{
    Q_UNUSED(scanType);
    QStringList args;

    args << "--max-filesize=200M";
    args << "--max-scansize=200M";

    // Only scan archives if user enabled it
    if (scanArchives())
        args << "--scan-archive=yes";
    else
        args << "--scan-archive=no";

    args << "--scan-mail=no";
    args << "--no-summary";

    // Disable unnecessary scanners for speed (per Fast.txt recommendations)
    if (!scanDocuments()) {
        args << "--scan-ole2=no";    // no Office docs
        args << "--scan-pdf=no";     // no PDF
    }

    // Database path
    QDir engineDir = QFileInfo(m_enginePath).absoluteDir();
    QStringList dbCandidates;
    dbCandidates << engineDir.absoluteFilePath("database");
    dbCandidates << QDir::cleanPath(engineDir.absoluteFilePath("../database"));
    for (const QString &candidate : dbCandidates) {
        if (QDir(candidate).exists()) {
            args << "--database=" + QDir::toNativeSeparators(candidate);
            break;
        }
    }
    return args;
}

// ── Collect scan targets ──────────────────────────────────────────
QStringList ClamAVEngine::collectTargets(ScanType scanType, const QString &customPath) const
{
    QStringList targets;
    switch (scanType) {
    case QuickScan:
        targets << QDir::homePath() + "/Downloads";
        targets << QDir::tempPath();
        targets << QDir::homePath() + "/AppData/Local/Temp";
        targets << "C:/Windows/Temp";
        targets << QDir::homePath() + "/AppData/Roaming";
        targets << QDir::homePath() + "/Desktop";
        targets << QDir::homePath() + "/AppData/Roaming/Microsoft/Windows/Start Menu/Programs/Startup";
        break;
    case FullScan:
        foreach (const QFileInfo &drive, QDir::drives())
            targets << drive.absoluteFilePath();
        break;
    case CustomScan:
        if (!customPath.isEmpty()) targets << customPath;
        break;
    }
    QStringList valid;
    for (const QString &t : targets)
        if (QFileInfo::exists(t)) valid << QDir::toNativeSeparators(t);
    return valid;
}

// ── Parse clamscan output ─────────────────────────────────────────
void ClamAVEngine::onReadyRead()
{
    if (!m_process) return;
    QByteArray data = m_process->readAll();
    m_buffer += QString::fromLocal8Bit(data);

    while (true) {
        int nl = m_buffer.indexOf('\n');
        if (nl < 0) break;
        QString line = m_buffer.left(nl).trimmed();
        m_buffer = m_buffer.mid(nl + 1);
        if (line.isEmpty()) continue;
        if (line.contains("ClamAV") || line.startsWith("Copyright")
            || line.startsWith("Loaded") || line.contains("LibClamAV")
            || line.contains("Database")) continue;

        static QRegularExpression foundRe(R"((.+):\s+(.+?)\s+FOUND\s*$)");
        static QRegularExpression okRe(R"((.+):\s+OK\s*$)");
        QRegularExpressionMatch match = foundRe.match(line);
        if (match.hasMatch()) {
            QString filePath = match.captured(1).trimmed();
            m_threatsFound++; m_scannedCount++;
            emit scanOutput(m_engineName,
                QString("⚠ [%1] %2").arg(match.captured(2).trimmed(), filePath));
            emit threatFound(m_engineName, filePath,
                QString("ClamAV: %1").arg(match.captured(2).trimmed()), 4);
            if (m_totalFiles > 0) {
                int pct = (int)((long long)m_scannedCount * 100 / m_totalFiles);
                if (pct != m_progress) { m_progress = pct; emit progressChanged(); }
            }
            m_currentFile = filePath; emit currentFileChanged();
            continue;
        }
        match = okRe.match(line);
        if (match.hasMatch()) {
            m_scannedCount++;
            if (m_totalFiles > 0) {
                int pct = (int)((long long)m_scannedCount * 100 / m_totalFiles);
                if (pct != m_progress) { m_progress = pct; emit progressChanged(); }
            }
            QString fp = match.captured(1).trimmed();
            m_currentFile = (m_totalFiles > 0 && m_scannedCount % 50 == 0)
                ? QString("[%1/%2] %3").arg(m_scannedCount).arg(m_totalFiles).arg(fp) : fp;
            emit currentFileChanged();
            continue;
        }
        emit scanOutput(m_engineName, line);
    }
}

// ── Scan finished ─────────────────────────────────────────────────
void ClamAVEngine::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (!m_buffer.trimmed().isEmpty())
        emit scanOutput(m_engineName, m_buffer.trimmed());

    if (m_fileListTemp) { m_fileListTemp->close(); m_fileListTemp->deleteLater(); m_fileListTemp = nullptr; }

    m_scanning = false;
    m_progress = 100;
    emit progressChanged(); emit scanningChanged();

    if (m_cancelled) {
        emit scanOutput(m_engineName, QStringLiteral("⏹ ClamAV 扫描已取消"));
        emit finished(m_engineName, -1, m_threatsFound);
    } else if (exitStatus == QProcess::CrashExit) {
        emit scanOutput(m_engineName, QStringLiteral("❌ ClamAV 进程崩溃"));
        emit finished(m_engineName, -3, m_threatsFound);
    } else {
        if (exitCode == 1)
            emit scanOutput(m_engineName, QString("✅ ClamAV 完成 — 发现 %1 个威胁").arg(m_threatsFound));
        else if (exitCode == 0)
            emit scanOutput(m_engineName, QStringLiteral("✅ ClamAV 完成 — 未发现威胁"));
        else
            emit scanOutput(m_engineName, QString("⚠ ClamAV 完成 (退出码 %1)").arg(exitCode));
        emit finished(m_engineName, exitCode, m_threatsFound);
    }

    if (m_process) { m_process->deleteLater(); m_process = nullptr; }
}

// ── Cancel ────────────────────────────────────────────────────────
void ClamAVEngine::cancel()
{
    m_cancelled = true;
    if (m_fileListTemp) { m_fileListTemp->close(); m_fileListTemp->deleteLater(); m_fileListTemp = nullptr; }
    if (m_process && m_process->state() != QProcess::NotRunning)
        m_process->kill();
}

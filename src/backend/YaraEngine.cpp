#include "YaraEngine.h"
#include <QCoreApplication>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QRegularExpression>
#include <QTimer>

YaraEngine::YaraEngine(QObject *parent)
    : ScanEngine(QStringLiteral("YARA"), parent)
{
    resolveEnginePath();
    m_ruleFiles = collectRuleFiles();
    
    // Pre-compute self-exclusion paths (normalize to forward slashes)
    m_appDir = QCoreApplication::applicationDirPath().toLower().replace('\\', '/');
    QStringList ruleDirs;
    ruleDirs << QCoreApplication::applicationDirPath() + "/engines/yara/rules";
    ruleDirs << QDir::cleanPath(QCoreApplication::applicationDirPath() + "/../deploy/engines/yara/rules");
    for (const QString &d : ruleDirs) {
        if (QDir(d).exists())
            m_ruleDirs << d.toLower().replace('\\', '/');
    }
}

// ── Path resolution ──────────────────────────────────────────────
void YaraEngine::resolveEnginePath()
{
    QStringList candidates;
    QString appDir = QCoreApplication::applicationDirPath();
    candidates << appDir + "/engines/yara/yara.exe";
    candidates << appDir + "/engines/yara/yara64.exe";
    candidates << QDir::cleanPath(appDir + "/../deploy/engines/yara/yara.exe");
    candidates << QDir::cleanPath(appDir + "/../deploy/engines/yara/yara64.exe");

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
    where.start("where", QStringList() << "yara.exe");
    where.waitForFinished(3000);
    if (where.exitCode() == 0) {
        QString out = QString::fromLocal8Bit(where.readAllStandardOutput()).trimmed();
        QStringList lines = out.split('\n', Qt::SkipEmptyParts);
        if (!lines.isEmpty() && QFileInfo::exists(lines.first().trimmed())) {
            m_enginePath = lines.first().trimmed();
            m_available = true; return;
        }
    }
    where.start("where", QStringList() << "yara64.exe");
    where.waitForFinished(3000);
    if (where.exitCode() == 0) {
        QString out = QString::fromLocal8Bit(where.readAllStandardOutput()).trimmed();
        QStringList lines = out.split('\n', Qt::SkipEmptyParts);
        if (!lines.isEmpty() && QFileInfo::exists(lines.first().trimmed())) {
            m_enginePath = lines.first().trimmed();
            m_available = true; return;
        }
    }
    m_available = false;
    m_enginePath.clear();
}

// ── Check if a path belongs to our own engine/rules ──────────────
bool YaraEngine::isSelfPath(const QString &filePath) const
{
    // Normalize: convert to lowercase, standardize separators
    QString lower = filePath.toLower();
    lower.replace('\\', '/');
    
    // Check if it's a YARA rule file anywhere (global exclusion)
    if (lower.endsWith(".yar") || lower.endsWith(".yara"))
        return true;
    
    // Check if path contains any self-identifying markers
    if (lower.contains("engines/yara"))
        return true;
    
    // Check against rule directories (normalized)
    for (const QString &rd : m_ruleDirs) {
        if (lower.startsWith(rd + "/") || lower == rd)
            return true;
    }
    
    // Check against app directory
    if (lower.startsWith(m_appDir + "/") || lower == m_appDir)
        return true;
    
    // Check deploy dir
    QString deployDir = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/../deploy").toLower().replace('\\', '/');
    if (lower.startsWith(deployDir + "/") || lower == deployDir)
        return true;
    
    // Check build dir
    QString buildDir = QCoreApplication::applicationDirPath().toLower().replace('\\', '/');
    if (lower.startsWith(buildDir + "/") || lower == buildDir)
        return true;
    
    return false;
}

// ── Collect rule files ──────────────────────────────────────────
QStringList YaraEngine::collectRuleFiles() const
{
    QStringList rules;
    QString appDir = QCoreApplication::applicationDirPath();
    QStringList ruleDirs;
    ruleDirs << appDir + "/engines/yara/rules";
    ruleDirs << QDir::cleanPath(appDir + "/../deploy/engines/yara/rules");

    for (const QString &dir : ruleDirs) {
        QDir d(dir);
        if (!d.exists()) continue;
        QDirIterator it(dir, {"*.yar", "*.yara"}, QDir::Files,
                        QDirIterator::Subdirectories);
        while (it.hasNext())
            rules << it.next();
    }
    return rules;
}

// ── Start scan ──────────────────────────────────────────────────
void YaraEngine::start(ScanType scanType, const QString &customPath)
{
    if (m_scanning) return;

    if (!m_available) {
        emit scanOutput(m_engineName, QStringLiteral("⚠ YARA 引擎不可用"));
        emit finished(m_engineName, -2, 0); return;
    }
    if (m_ruleFiles.isEmpty()) {
        emit scanOutput(m_engineName, QStringLiteral("⚠ YARA 引擎无规则文件"));
        emit finished(m_engineName, -2, 0); return;
    }

    m_scanning = true;
    m_progress = 5;
    m_cancelled = false;
    m_threatsFound = 0;
    m_scannedCount = 0;
    m_totalFiles = 0;
    m_buffer.clear();
    emit scanningChanged(); emit progressChanged();
    emit scanOutput(m_engineName, QStringLiteral("🔍 YARA 引擎开始扫描..."));
    emit scanOutput(m_engineName,
        QString("📋 加载 %1 个规则文件").arg(m_ruleFiles.size()));

    QStringList targets = collectTargets(scanType, customPath);
    if (targets.isEmpty()) {
        emit scanOutput(m_engineName, QStringLiteral("⚠ 没有可扫描的目标"));
        m_scanning = false; emit scanningChanged();
        emit finished(m_engineName, 0, 0); return;
    }

    // ── Pre-count files for real progress ────────────────────
    emit scanOutput(m_engineName, QStringLiteral("📊 正在统计文件数量..."));
    m_currentFile = QStringLiteral("统计文件中...");
    emit currentFileChanged();
    QCoreApplication::processEvents();

    // Count files in targets (only executable types, matching what YARA will scan)
    QStringList exts = {"*.exe","*.dll","*.scr","*.bat","*.cmd","*.ps1","*.vbs",
                        "*.js","*.jar","*.msi","*.com","*.pif","*.sys","*.drv",
                        "*.zip","*.7z","*.rar","*.docm","*.xlsm","*.pptm","*.pdf"};
    int dirIdx = 0;
    for (const QString &target : targets) {
        dirIdx++;
        if (dirIdx % 2 == 0) {
            m_currentFile = QString("统计: %1/%2 目录...").arg(dirIdx).arg(targets.size());
            emit currentFileChanged();
            QCoreApplication::processEvents();
        }
        for (const QString &pattern : exts) {
            QDirIterator it(target, {pattern}, QDir::Files,
                            QDirIterator::Subdirectories | QDirIterator::FollowSymlinks);
            while (it.hasNext()) {
                it.next();
                m_totalFiles++;
                if (m_totalFiles % 500 == 0) {
                    m_currentFile = QString("已统计 %1 个文件...").arg(m_totalFiles);
                    emit currentFileChanged();
                    QCoreApplication::processEvents();
                }
            }
        }
    }
    emit scanOutput(m_engineName,
        QString("📊 统计完成: %1 个可扫描文件").arg(m_totalFiles));
    m_currentFile = QString("共 %1 个文件，开始扫描...").arg(m_totalFiles);
    emit currentFileChanged();

    // Build args: flags + rule files + targets
    QStringList args;
    args << "-r" << "-w" << "-s";  // -s for matched strings (debug detail)
    for (const QString &rule : m_ruleFiles)
        args << rule;
    args << targets;

    emit scanOutput(m_engineName,
        QString("⚙ 命令: yara -r -w -s <规则> %1 个目标").arg(targets.size()));

    m_process = new QProcess(this);
    m_process->setProcessChannelMode(QProcess::MergedChannels);
    connect(m_process, &QProcess::readyRead, this, &YaraEngine::onReadyRead);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &YaraEngine::onProcessFinished);

    m_currentFile = QStringLiteral("正在启动 YARA...");
    emit currentFileChanged();

    // Progress: real file-based estimation
    // YARA doesn't output per-file OK, so we estimate based on scan output frequency
    m_progressTimer = new QTimer(this);
    m_progressTimer->setInterval(2000);
    connect(m_progressTimer, &QTimer::timeout, this, [this]() {
        if (m_scanning && m_totalFiles > 0) {
            // Estimate: assume linear scan rate, cap at 90% until complete
            int estimated = qMin(90, m_progress + qMax(1, (90 - m_progress) / 10));
            if (estimated > m_progress) {
                m_progress = estimated;
                emit progressChanged();
            }
        }
    });
    m_progressTimer->start();

    m_process->start(m_enginePath, args);
}

// ── Collect scan targets ────────────────────────────────────────
QStringList YaraEngine::collectTargets(ScanType scanType, const QString &customPath) const
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

// ── Parse yara output ───────────────────────────────────────────
void YaraEngine::onReadyRead()
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

        // Skip indented detail lines (matched strings)
        if (line.startsWith("0x") || line.startsWith("  ") || line.startsWith('\t'))
            continue;

        // Skip YARA header/error messages
        if (line.startsWith("yara: ") || line.startsWith("error: ")
            || line.startsWith("warning: ")) {
            emit scanOutput(m_engineName, line);
            continue;
        }

        // Match format: "rule_name  file_path"
        int spaceIdx = line.indexOf(' ');
        if (spaceIdx > 0) {
            QString ruleName = line.left(spaceIdx).trimmed();
            QString filePath = line.mid(spaceIdx + 1).trimmed();

            if (!filePath.isEmpty()) {
                // ── Self-exclusion: skip our own files ──────────
                if (isSelfPath(filePath)) {
                    emit scanOutput(m_engineName,
                        QString("⏭ 跳过自身文件: %1").arg(filePath));
                    continue;
                }

                m_threatsFound++;
                m_scannedCount++;

                // Update progress based on match count (matches are rare)
                if (m_totalFiles > 0 && m_scannedCount > 0) {
                    // A match means we've scanned at least this far
                    // Keep a rough estimate
                }

                emit scanOutput(m_engineName,
                    QString("⚠ [%1] %2").arg(ruleName, filePath));
                emit threatFound(m_engineName, filePath,
                    QString("YARA: %1").arg(ruleName), 4);
            }
        } else {
            emit scanOutput(m_engineName, line);
        }
    }
}

// ── Scan finished ───────────────────────────────────────────────
void YaraEngine::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (!m_buffer.trimmed().isEmpty()) {
        QStringList lines = m_buffer.split('\n', Qt::SkipEmptyParts);
        for (const QString &line : lines) {
            if (line.isEmpty() || line.startsWith("0x") || line.startsWith("  ")
                || line.startsWith('\t')) continue;
            if (line.startsWith("yara: ") || line.startsWith("error: ")) continue;
            int si = line.indexOf(' ');
            if (si > 0) {
                QString rn = line.left(si).trimmed();
                QString fp = line.mid(si + 1).trimmed();
                if (!fp.isEmpty() && !isSelfPath(fp)) {
                    m_threatsFound++;
                    emit threatFound(m_engineName, fp,
                        QString("YARA: %1").arg(rn), 4);
                }
            }
        }
    }

    if (m_progressTimer) {
        m_progressTimer->stop();
        m_progressTimer->deleteLater();
        m_progressTimer = nullptr;
    }

    m_scanning = false;
    m_progress = 100;
    emit progressChanged(); emit scanningChanged();

    if (m_cancelled) {
        emit scanOutput(m_engineName, QStringLiteral("⏹ YARA 扫描已取消"));
        emit finished(m_engineName, -1, m_threatsFound);
    } else if (exitStatus == QProcess::CrashExit) {
        emit scanOutput(m_engineName, QStringLiteral("❌ YARA 进程崩溃"));
        emit finished(m_engineName, -3, m_threatsFound);
    } else {
        if (exitCode == 0)
            emit scanOutput(m_engineName, QString("✅ YARA 完成 — 未发现威胁 (扫描 %1 文件)").arg(m_totalFiles));
        else
            emit scanOutput(m_engineName,
                QString("✅ YARA 完成 — 发现 %1 个威胁").arg(m_threatsFound));
        emit finished(m_engineName, exitCode, m_threatsFound);
    }

    if (m_process) { m_process->deleteLater(); m_process = nullptr; }
}

// ── Cancel ──────────────────────────────────────────────────────
void YaraEngine::cancel()
{
    m_cancelled = true;
    if (m_progressTimer) { m_progressTimer->stop(); m_progressTimer->deleteLater(); m_progressTimer = nullptr; }
    if (m_process && m_process->state() != QProcess::NotRunning)
        m_process->kill();
}

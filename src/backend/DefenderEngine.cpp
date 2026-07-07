#include "DefenderEngine.h"
#include <QCoreApplication>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QTimer>
#include <QStandardPaths>
#include <algorithm>

DefenderEngine::DefenderEngine(QObject *parent)
    : ScanEngine(QStringLiteral("Windows Defender"), parent)
{
    m_mpPath = findMpCmdRun();
}

// ── Find MpCmdRun.exe ─────────────────────────────────────────
QString DefenderEngine::findMpCmdRun()
{
    // 1. System PATH
    QProcess where;
    where.start("where", QStringList() << "MpCmdRun.exe");
    where.waitForFinished(5000);
    if (where.exitCode() == 0) {
        QString output = QString::fromLocal8Bit(where.readAllStandardOutput()).trimmed();
        QStringList lines = output.split('\n', Qt::SkipEmptyParts);
        if (!lines.isEmpty()) {
            QString p = lines.first().trimmed();
            if (QFile::exists(p)) return QDir::toNativeSeparators(p);
        }
    }

    // 2. Standard paths
    QStringList candidates;
    candidates << "C:/Program Files/Windows Defender/MpCmdRun.exe";
    candidates << "C:/Program Files (x86)/Windows Defender/MpCmdRun.exe";

    // 3. Platform versioned path
    QString platformBase = "C:/ProgramData/Microsoft/Windows Defender/Platform";
    QDir platformDir(platformBase);
    if (platformDir.exists()) {
        QStringList versions = platformDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot,
                                                      QDir::Name | QDir::Reversed);
        for (const QString &ver : versions) {
            QString p = platformBase + "/" + ver + "/MpCmdRun.exe";
            if (QFile::exists(p)) return QDir::toNativeSeparators(p);
        }
    }

    for (const QString &p : candidates) {
        if (QFile::exists(p)) return QDir::toNativeSeparators(p);
    }
    return QString();
}

// ── Start scan ────────────────────────────────────────────────
void DefenderEngine::start(ScanType scanType, const QString &customPath)
{
    if (m_scanning) return;

    if (m_mpPath.isEmpty()) {
        emit scanOutput(m_engineName,
            QStringLiteral("❌ Windows Defender 命令行工具未找到！"));
        emit finished(m_engineName, -3, 0);
        return;
    }

    m_scanning = true;
    m_progress = 0;
    m_lineCount = 0;
    m_threatsFound = 0;
    m_cancelled = false;
    m_currentTargetIndex = 0;
    m_scanTargets.clear();
    m_savedScanType = scanType;
    m_savedCustomPath = customPath;
    emit scanningChanged();
    emit progressChanged();

    emit scanOutput(m_engineName,
        QString("🛡 MpCmdRun: %1").arg(m_mpPath));

    // ── QuickScan / FullScan: use native Windows Defender scan ──
    if (scanType == QuickScan || scanType == FullScan) {
        int nativeScanType = (scanType == QuickScan) ? 1 : 2;
        const char *typeName = (scanType == QuickScan) ? "快速扫描" : "全面扫描";

        emit scanOutput(m_engineName,
            QString("🔍 启动 Windows Defender %1 (ScanType %2)...")
                .arg(typeName).arg(nativeScanType));

        m_currentFile = QString("正在运行 %1...").arg(typeName);
        emit currentFileChanged();

        m_process = new QProcess(this);
        m_process->setProcessChannelMode(QProcess::MergedChannels);
        connect(m_process, &QProcess::readyReadStandardOutput,
                this, &DefenderEngine::onReadyRead);
        connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this, &DefenderEngine::onFinished);
        connect(m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError) {
            emit scanOutput(m_engineName,
                QString("❌ 进程错误: %1").arg(m_process ? m_process->errorString() : ""));
        });

        // Native scan: no -File needed
        m_process->start(m_mpPath,
            QStringList() << "-Scan" << "-ScanType" << QString::number(nativeScanType));

        // Progress timer for native scan (can take minutes)
        m_progressTimer = new QTimer(this);
        m_progressTimer->setInterval(3000);
        connect(m_progressTimer, &QTimer::timeout, this, [this]() {
            if (m_scanning && m_progress < 90) {
                m_progress += qMax(1, (90 - m_progress) / 15);
                emit progressChanged();
            }
        });
        m_progressTimer->start();
        return;
    }

    // ── CustomScan: scan specific directories ─────────────────
    if (customPath.isEmpty()) {
        emit scanOutput(m_engineName, QStringLiteral("⚠ 自定义扫描需要指定路径"));
        m_scanning = false; emit scanningChanged();
        emit finished(m_engineName, 0, 0);
        return;
    }

    m_scanTargets << customPath;
    emit scanOutput(m_engineName,
        QString("🔍 自定义扫描 %1 个目标").arg(m_scanTargets.size()));

    for (int i = 0; i < m_scanTargets.size(); ++i)
        emit scanOutput(m_engineName,
            QString("  [%1] %2").arg(i + 1).arg(m_scanTargets[i]));

    scanNextTarget();
}

void DefenderEngine::scanNextTarget()
{
    if (m_cancelled || m_currentTargetIndex >= m_scanTargets.size()) {
        m_scanning = false;
        if (m_progressTimer) { m_progressTimer->stop(); m_progressTimer->deleteLater(); m_progressTimer = nullptr; }
        m_currentFile = QStringLiteral("扫描完成");
        m_progress = 100;
        emit currentFileChanged(); emit progressChanged(); emit scanningChanged();
        emit scanOutput(m_engineName,
            QString("✅ Defender 完成 — 扫描 %1 个目标, 威胁: %2")
                .arg(m_scanTargets.size()).arg(m_threatsFound));
        emit finished(m_engineName, 0, m_threatsFound);
        return;
    }

    QString target = m_scanTargets[m_currentTargetIndex++];
    m_currentFile = target;
    m_progress = (m_currentTargetIndex - 1) * 100 / m_scanTargets.size();
    emit currentFileChanged(); emit progressChanged();
    emit scanOutput(m_engineName,
        QString("🔍 [%1/%2] MpCmdRun -Scan -ScanType 3 -File \"%3\"")
            .arg(m_currentTargetIndex).arg(m_scanTargets.size()).arg(target));

    m_process = new QProcess(this);
    m_process->setProcessChannelMode(QProcess::MergedChannels);
    connect(m_process, &QProcess::readyReadStandardOutput,
            this, &DefenderEngine::onReadyRead);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &DefenderEngine::onFinished);
    connect(m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError) {
        emit scanOutput(m_engineName, QStringLiteral("❌ MpCmdRun 启动失败"));
        if (m_process) { m_process->deleteLater(); m_process = nullptr; }
        scanNextTarget();
    });

    m_process->start(m_mpPath,
        QStringList() << "-Scan" << "-ScanType" << "3" << "-File" << target);

    m_targetProgress = m_currentTargetIndex * 100 / m_scanTargets.size();
    if (!m_progressTimer) {
        m_progressTimer = new QTimer(this);
        m_progressTimer->setInterval(2000);
        connect(m_progressTimer, &QTimer::timeout, this, [this]() {
            if (m_scanning && m_progress < m_targetProgress) {
                m_progress = qMin(m_targetProgress, m_progress + 3);
                emit progressChanged();
            }
        });
    }
    m_progressTimer->start();
}

void DefenderEngine::cancel()
{
    m_cancelled = true;
    m_scanTargets.clear();
    if (m_process && m_scanning) {
        m_process->disconnect();
        m_process->kill();
        m_process->deleteLater();
        m_process = nullptr;
    }
    if (m_progressTimer) { m_progressTimer->stop(); m_progressTimer->deleteLater(); m_progressTimer = nullptr; }
    m_scanning = false;
    m_currentFile = QStringLiteral("已取消");
    m_progress = 0;
    emit currentFileChanged(); emit progressChanged(); emit scanningChanged();
    emit scanOutput(m_engineName, QStringLiteral("⏹ 扫描已取消"));
    emit finished(m_engineName, -1, -1);
}

void DefenderEngine::onReadyRead()
{
    if (!m_process) return;
    QString data = QString::fromLocal8Bit(m_process->readAll());
    QStringList lines = data.split(QRegularExpression("[\r\n]+"), Qt::SkipEmptyParts);
    for (const QString &line : lines) {
        QString trimmed = line.trimmed();
        if (trimmed.isEmpty()) continue;
        parseOutputLine(trimmed);
        emit scanOutput(m_engineName, trimmed);
        m_lineCount++;
        if (m_lineCount % 5 == 0 && !m_scanTargets.isEmpty()) {
            m_currentFile = QString("%1 (处理中...)").arg(
                m_currentTargetIndex > 0 ? m_scanTargets[m_currentTargetIndex - 1] : "系统扫描");
            emit currentFileChanged();
        }
    }
}

void DefenderEngine::onFinished(int exitCode)
{
    if (m_progressTimer) m_progressTimer->stop();

    if (m_process) { m_process->deleteLater(); m_process = nullptr; }
    if (m_cancelled) return;

    // For native QuickScan/FullScan, we're done after one run
    if (m_scanTargets.isEmpty()) {
        m_scanning = false;
        m_progress = 100;
        emit progressChanged(); emit scanningChanged();
        emit scanOutput(m_engineName,
            QString("✅ Defender 扫描完成 (退出码: %1), 威胁: %2").arg(exitCode).arg(m_threatsFound));
        emit finished(m_engineName, exitCode, m_threatsFound);
        return;
    }

    // Custom scan: move to next target
    emit scanOutput(m_engineName,
        QString("  ✓ 完成 (退出码: %1)").arg(exitCode));
    scanNextTarget();
}

void DefenderEngine::parseOutputLine(const QString &line)
{
    // Threat detection (English)
    static QRegularExpression threatRe(
        R"(Threat\s+(?:found|detected)\s*:?\s*(.+?)\s+file:?/*(.+))",
        QRegularExpression::CaseInsensitiveOption);
    // Chinese
    static QRegularExpression threatReCN(
        R"(检测到威胁|威胁\s*[:：]\s*(.+?)\s*(?:文件|路径)\s*[:：]\s*(.+))");

    auto match = threatRe.match(line);
    if (match.hasMatch()) {
        m_threatsFound++;
        emit threatFound(m_engineName, match.captured(2).trimmed(),
            match.captured(1).trimmed(), 3);
        return;
    }
    match = threatReCN.match(line);
    if (match.hasMatch()) {
        m_threatsFound++;
        emit threatFound(m_engineName, match.captured(2).trimmed(),
            match.captured(1).trimmed(), 3);
        return;
    }

    static QRegularExpression scanningRe(
        R"(Scanning\s+(.+?)(?:\.{3}|$))",
        QRegularExpression::CaseInsensitiveOption);
    auto scanMatch = scanningRe.match(line);
    if (scanMatch.hasMatch()) {
        m_currentFile = scanMatch.captured(1).trimmed();
        emit currentFileChanged();
    }
}

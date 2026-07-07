#include "DefenderScanner.h"
#include <QCoreApplication>
#include <QDir>
#include <QDateTime>
#include <QDate>
#include <QRegularExpression>
#include <QSettings>
#include <QTimer>

DefenderScanner::DefenderScanner(QObject *parent)
    : QObject(parent)
{
    QSettings s("ZackDefender", "ZackDefender");

    // ── Restore last scan info from previous session ───────
    m_lastScanType = s.value("lastScan/type").toString();
    m_lastScanTime = s.value("lastScan/time").toString();
    m_lastThreatCount = s.value("lastScan/threats").toInt();

    // ── Install date & protection days ────────────────────
    QString installStr = s.value("installDate").toString();
    if (installStr.isEmpty()) {
        // First run — record today
        installStr = QDate::currentDate().toString(Qt::ISODate);
        s.setValue("installDate", installStr);
    }
    QDate installDate = QDate::fromString(installStr, Qt::ISODate);
    m_protectionDays = installDate.daysTo(QDate::currentDate());
}

QVariantList DefenderScanner::scanLog() const
{
    QVariantList result;
    for (const auto &entry : m_scanLog)
        result.append(QVariant::fromValue(entry));
    return result;
}

QVariantList DefenderScanner::threats() const
{
    QVariantList result;
    for (const auto &entry : m_threats)
        result.append(QVariant::fromValue(entry));
    return result;
}

void DefenderScanner::appendLog(const QString &text, bool isThreat,
                                 const QString &filePath, const QString &threatName)
{
    QVariantMap entry;
    entry["text"] = text;
    entry["isThreat"] = isThreat;
    if (!filePath.isEmpty()) entry["filePath"] = filePath;
    if (!threatName.isEmpty()) entry["threatName"] = threatName;
    m_scanLog.append(entry);
    m_lineCount++;

    // ── Incremental progress ──────────────────────────────
    // Estimate: quick~200 lines, full~800, custom~500
    int estimatedTotal = (m_currentScanType == FullScan) ? 800
                       : (m_currentScanType == QuickScan) ? 200
                       : 500;
    int newProgress = qMin(95, m_lineCount * 95 / estimatedTotal);
    if (newProgress != m_progress) {
        m_progress = newProgress;
        emit progressChanged();
    }

    emit scanLogChanged();
    emit scanOutput(text);
}

QString DefenderScanner::mpCmdRunPath()
{
    const QStringList candidates = {
        "C:/Program Files/Windows Defender/MpCmdRun.exe",
        "C:/Program Files (x86)/Windows Defender/MpCmdRun.exe",
    };
    for (const auto &p : candidates)
        if (QFile::exists(p))
            return p;
    return "MpCmdRun.exe";
}

void DefenderScanner::startScan(int scanType, const QString &customPath)
{
    if (m_scanning) return;

    m_scanning = true;
    m_progress = 1;
    m_lineCount = 0;
    m_threatsFound = 0;
    m_currentFile.clear();
    m_scanLog.clear();
    m_threats.clear();
    m_currentScanType = scanType;
    emit progressChanged();
    emit scanLogChanged();
    emit scanningChanged();

    m_process = new QProcess(this);
    m_process->setProcessChannelMode(QProcess::MergedChannels);  // stderr→stdout
    connect(m_process, &QProcess::readyReadStandardOutput, this, &DefenderScanner::onReadyRead);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &DefenderScanner::onFinished);

    QStringList args;
    args << "-Scan" << "-ScanType" << QString::number(scanType);
    if (scanType == CustomScan && !customPath.isEmpty())
        args << "-File" << customPath;

    const char *typeNames[] = {"", "快速扫描", "全面扫描", "自定义扫描"};
    appendLog(QString("🔍 开始%1...").arg(typeNames[scanType]), false);
    m_process->start(mpCmdRunPath(), args);

    // Progress timer — gently bump progress if no output for a while
    m_progressTimer = new QTimer(this);
    m_progressTimer->setInterval(3000);
    connect(m_progressTimer, &QTimer::timeout, this, [this]() {
        if (m_scanning && m_progress < 90 && m_progressTimer) {
            m_progress = qMin(90, m_progress + 2);
            emit progressChanged();
        }
    });
    m_progressTimer->start();
}

void DefenderScanner::cancelScan()
{
    if (m_process && m_scanning) {
        m_scanning = false;

        // Disconnect finished signal so kill() doesn't trigger onFinished
        m_process->disconnect();
        m_process->kill();
        m_process->deleteLater();
        m_process = nullptr;

        if (m_progressTimer) { m_progressTimer->stop(); m_progressTimer->deleteLater(); m_progressTimer = nullptr; }

        m_currentFile = QStringLiteral("已取消");
        m_progress = 0;
        emit currentFileChanged();
        emit progressChanged();
        emit scanningChanged();
        appendLog("⏹ 扫描已取消", false);

        // Emit scanFinished with -1 to indicate cancellation
        emit scanFinished(-1, -1);
    }
}

void DefenderScanner::onReadyRead()
{
    if (!m_process) return;
    QString data = QString::fromLocal8Bit(m_process->readAll());
    QStringList lines = data.split(QRegularExpression("[\r\n]+"), Qt::SkipEmptyParts);
    for (const QString &line : lines) {
        QString trimmed = line.trimmed();
        if (trimmed.isEmpty()) continue;

        parseOutputLine(trimmed);
        appendLog(trimmed, false);

        if (m_currentFile.isEmpty() && trimmed.length() > 3) {
            m_currentFile = trimmed.left(120);
            emit currentFileChanged();
        }
    }
}

void DefenderScanner::onFinished(int exitCode)
{
    m_scanning = false;

    if (m_progressTimer) { m_progressTimer->stop(); m_progressTimer->deleteLater(); m_progressTimer = nullptr; }

    // ── Smoothly animate progress to 100% ─────────────────
    QTimer *finishTimer = new QTimer(this);
    finishTimer->setInterval(30);
    connect(finishTimer, &QTimer::timeout, this, [this, finishTimer]() {
        if (m_progress >= 100) {
            finishTimer->stop();
            finishTimer->deleteLater();
            m_currentFile = QStringLiteral("扫描完成");
            emit currentFileChanged();
            return;
        }
        // Increment faster as we get closer to 100
        int step = qMax(1, (100 - m_progress) / 10);
        m_progress = qMin(100, m_progress + step);
        emit progressChanged();
    });
    finishTimer->start();

    // Fire scanFinished signal after short delay so UI sees 100%
    QTimer::singleShot(400, this, [this, exitCode]() {
        appendLog(QString("✅ 扫描完成 — 退出码 %1, 发现威胁: %2")
                      .arg(exitCode).arg(m_threatsFound), false);

        // ── Update persistent last-scan info ──────────────────
        m_lastThreatCount = m_threatsFound;
        m_lastScanTime = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
        switch (m_currentScanType) {
            case QuickScan:  m_lastScanType = QStringLiteral("快速扫描"); break;
            case FullScan:   m_lastScanType = QStringLiteral("全面扫描"); break;
            case CustomScan: m_lastScanType = QStringLiteral("自定义扫描"); break;
            default:         m_lastScanType = QStringLiteral("扫描"); break;
        }
        emit lastScanTypeChanged();
        emit lastScanTimeChanged();
        emit lastThreatCountChanged();

        // ── Persist to survive app restarts ───────────────────
        QSettings s("ZackDefender", "ZackDefender");
        s.setValue("lastScan/type", m_lastScanType);
        s.setValue("lastScan/time", m_lastScanTime);
        s.setValue("lastScan/threats", m_lastThreatCount);

        emit scanFinished(exitCode, m_threatsFound);

        m_process->deleteLater();
        m_process = nullptr;
    });

    emit scanningChanged();
}

void DefenderScanner::parseOutputLine(const QString &line)
{
    // ── Threat detection patterns ──────────────────────────
    static QRegularExpression threatRe(
        R"(Threat\s+(?:found|detected)\s*:?\s*(.+?)\s+file:?/*(.+))",
        QRegularExpression::CaseInsensitiveOption
    );
    // Alternative: "Threat  : VirusName"  "File: path"
    static QRegularExpression threatRe2(
        R"(Threat\s*:\s*(.+))",
        QRegularExpression::CaseInsensitiveOption
    );

    // ── File/path patterns ─────────────────────────────────
    static QRegularExpression scanningRe(
        R"(Scanning\s+(.+?)(?:\.{3}|$))",
        QRegularExpression::CaseInsensitiveOption
    );
    static QRegularExpression pathRe(
        R"(([A-Za-z]:\\[^\s,;]+))"  // Windows path like C:\...
    );
    static QRegularExpression fileRe(
        R"(file:?\s*(.+))",
        QRegularExpression::CaseInsensitiveOption
    );

    // Check for threats
    auto match = threatRe.match(line);
    if (match.hasMatch()) {
        m_threatsFound++;
        QString threat = match.captured(1).trimmed();
        QString path = match.captured(2).trimmed();
        QVariantMap entry;
        entry["text"] = QString("⚠ 发现威胁: %1 → %2").arg(threat, path);
        entry["isThreat"] = true;
        entry["filePath"] = path;
        entry["threatName"] = threat;
        m_scanLog.append(entry);
        m_threats.append(entry);
        emit scanLogChanged();
        emit threatFound(path, threat);
        emit threatsChanged();
        return;
    }

    // Check for "Scanning ..." pattern
    auto scanMatch = scanningRe.match(line);
    if (scanMatch.hasMatch()) {
        m_currentFile = scanMatch.captured(1).trimmed();
        emit currentFileChanged();
        return;
    }

    // Check for any Windows path in the line
    auto pathMatch = pathRe.match(line);
    if (pathMatch.hasMatch()) {
        m_currentFile = pathMatch.captured(1).trimmed();
        emit currentFileChanged();
    }
}

void DefenderScanner::clearThreats()
{
    if (m_threats.isEmpty()) return;
    m_threats.clear();
    emit threatsChanged();
}

void DefenderScanner::removeThreat(const QString &filePath)
{
    for (int i = 0; i < m_threats.size(); ++i) {
        if (m_threats[i]["filePath"].toString() == filePath) {
            m_threats.removeAt(i);
            emit threatsChanged();
            return;
        }
    }
}

void DefenderScanner::resetData()
{
    QSettings s("ZackDefender", "ZackDefender");
    s.remove("installDate");
    s.remove("lastScan/type");
    s.remove("lastScan/time");
    s.remove("lastScan/threats");

    // Reset in-memory state
    m_lastScanType.clear();
    m_lastScanTime.clear();
    m_lastThreatCount = 0;
    m_protectionDays = 0;

    emit lastScanTypeChanged();
    emit lastScanTimeChanged();
    emit lastThreatCountChanged();
    emit protectionDaysChanged();
}

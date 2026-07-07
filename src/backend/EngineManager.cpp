#include "EngineManager.h"
#include "ScanEngine.h"
#include "DefenderEngine.h"
#include "HashEngine.h"
#include "ClamAVEngine.h"
#include "YaraEngine.h"
#include <QCoreApplication>
#include <QDateTime>
#include <QDate>
#include <QDir>
#include <QSettings>
#include <QTimer>

EngineManager::EngineManager(QObject *parent)
    : QObject(parent)
{
    // ── Create engines ──────────────────────────────────────
    addEngine(new DefenderEngine(this));
    addEngine(new HashEngine(this));
    addEngine(new ClamAVEngine(this));
    addEngine(new YaraEngine(this));

    // ── Restore last scan info ──────────────────────────────
    QSettings s("ZackDefender", "ZackDefender");
    m_lastScanType = s.value("lastScan/type").toString();
    m_lastScanTime = s.value("lastScan/time").toString();
    m_lastThreatCount = s.value("lastScan/threats").toInt();

    // ── Protection days ─────────────────────────────────────
    QString installStr = s.value("installDate").toString();
    if (installStr.isEmpty()) {
        installStr = QDate::currentDate().toString(Qt::ISODate);
        s.setValue("installDate", installStr);
    }
    QDate installDate = QDate::fromString(installStr, Qt::ISODate);
    m_protectionDays = installDate.daysTo(QDate::currentDate());
}

void EngineManager::addEngine(ScanEngine *engine)
{
    m_engines.append(engine);

    connect(engine, &ScanEngine::threatFound,
            this, &EngineManager::onEngineThreat);
    connect(engine, &ScanEngine::finished,
            this, &EngineManager::onEngineFinished);
    connect(engine, &ScanEngine::scanOutput,
            this, &EngineManager::onEngineOutput);
    connect(engine, &ScanEngine::currentFileChanged,
            this, &EngineManager::currentFileChanged);
    connect(engine, &ScanEngine::progressChanged,
            this, &EngineManager::progressChanged);

    emit engineNamesChanged();
    emit activeEngineCountChanged();
}

bool EngineManager::isEngineEnabled(const QString &name) const
{
    for (auto *e : m_engines) {
        if (e->engineName() == name)
            return e->isEnabled();
    }
    return false;
}

void EngineManager::setEngineEnabled(const QString &name, bool enabled)
{
    for (auto *e : m_engines) {
        if (e->engineName() == name) {
            e->setEnabled(enabled);
            emit activeEngineCountChanged();
            return;
        }
    }
}

QObject *EngineManager::engineByName(const QString &name) const
{
    for (auto *e : m_engines) {
        if (e->engineName() == name)
            return e;
    }
    return nullptr;
}

int EngineManager::activeEngineCount() const
{
    int count = 0;
    for (auto *e : m_engines) {
        if (e->isEnabled()) count++;
    }
    return count;
}

QStringList EngineManager::engineNames() const
{
    QStringList names;
    // Hash engine first (fast pre-scan), then Defender
    for (auto *e : m_engines)
        names << e->engineName();
    return names;
}

int EngineManager::progress() const
{
    if (m_engines.isEmpty()) return 0;
    int total = 0;
    int running = 0;
    for (auto *e : m_engines) {
        if (!e->isEnabled()) continue;
        if (e->isScanning()) {
            total += e->progress();
            running++;
        }
    }
    if (running > 0) return total / running;
    // Engines starting up — show small progress to indicate activity
    return m_scanning ? 5 : 0;
}

QString EngineManager::currentFile() const
{
    QStringList active;
    QString fallback;

    for (auto *e : m_engines) {
        if (!e->isScanning()) continue;

        QString cf = e->currentFile();
        if (!cf.isEmpty()) {
            // Prefer engines showing an actual file path (contains \ or /)
            // over those still showing status messages like "正在启动..."
            if (cf.contains(QStringLiteral(":\\")) || cf.contains(QLatin1Char('/'))) {
                return QStringLiteral("[%1] %2").arg(e->engineName(), cf);
            }
            if (fallback.isEmpty())
                fallback = QStringLiteral("[%1] %2").arg(e->engineName(), cf);
        }
        active << e->engineName();
    }

    if (!fallback.isEmpty()) return fallback;
    if (!active.isEmpty())
        return QStringLiteral("正在启动: %1...").arg(active.join(", "));
    return {};
}

QVariantList EngineManager::scanLog() const
{
    QVariantList result;
    for (const auto &entry : m_scanLog)
        result.append(QVariant::fromValue(entry));
    return result;
}

QVariantList EngineManager::threats() const
{
    QVariantList result;
    for (const auto &entry : m_threats)
        result.append(QVariant::fromValue(entry));
    return result;
}

void EngineManager::appendLog(const QString &text, bool isThreat,
                               const QString &filePath, const QString &threatName)
{
    QVariantMap entry;
    entry["text"] = text;
    entry["isThreat"] = isThreat;
    if (!filePath.isEmpty()) entry["filePath"] = filePath;
    if (!threatName.isEmpty()) entry["threatName"] = threatName;
    m_scanLog.append(entry);
    emit scanLogChanged();
}

void EngineManager::startScan(int scanType, const QString &customPath)
{
    if (m_scanning) return;

    m_scanning = true;
    m_threatsFound = 0;
    m_enginesRunning = 0;
    m_threats.clear();
    m_seenThreats.clear();
    m_scanLog.clear();
    m_currentScanType = scanType;
    m_pendingScanType = static_cast<ScanEngine::ScanType>(scanType);
    m_pendingCustomPath = customPath;
    emit scanningChanged();
    emit scanLogChanged();
    emit threatsChanged();

    // Build ordered queue: Hash → ClamAV → YARA → Defender
    const char *orderedNames[] = {
        "哈希检测", "ClamAV", "YARA", "Windows Defender"
    };
    m_scanQueue.clear();
    for (const char *name : orderedNames) {
        for (int i = 0; i < m_engines.size(); ++i) {
            if (m_engines[i]->engineName() == QString::fromUtf8(name)
                && m_engines[i]->isEnabled()) {
                m_scanQueue.append(i);
                break;
            }
        }
    }

    const char *typeNames[] = {"", "快速扫描", "全面扫描", "自定义扫描"};
    appendLog(QString("🔍 启动顺序扫描 — %1 (%2 个引擎: %3)")
                  .arg(typeNames[scanType])
                  .arg(m_scanQueue.size())
                  .arg([this]() {
                      QStringList names;
                      for (int idx : m_scanQueue)
                          names << m_engines[idx]->engineName();
                      return names.join(" → ");
                  }()));

    startNextEngine();
}

void EngineManager::startNextEngine()
{
    if (m_scanQueue.isEmpty()) {
        // All engines done
        m_scanning = false;
        emit scanningChanged();

        appendLog(QString("✅ 所有引擎扫描完成 — 共发现 %1 个威胁").arg(m_threatsFound));

        // Update persistent info
        m_lastThreatCount = m_threatsFound;
        m_lastScanTime = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
        switch (m_currentScanType) {
            case 1: m_lastScanType = QStringLiteral("快速扫描 (顺序引擎)"); break;
            case 2: m_lastScanType = QStringLiteral("全面扫描 (顺序引擎)"); break;
            case 3: m_lastScanType = QStringLiteral("自定义扫描 (顺序引擎)"); break;
        }
        emit lastScanTypeChanged();
        emit lastScanTimeChanged();
        emit lastThreatCountChanged();

        QSettings s("ZackDefender", "ZackDefender");
        s.setValue("lastScan/type", m_lastScanType);
        s.setValue("lastScan/time", m_lastScanTime);
        s.setValue("lastScan/threats", m_lastThreatCount);

        emit scanFinished(0, m_threatsFound);
        return;
    }

    int idx = m_scanQueue.takeFirst();
    ScanEngine *engine = m_engines[idx];
    m_enginesRunning = 1;  // only one at a time now

    appendLog(QString("🔍 启动引擎 [%1]...").arg(engine->engineName()));
    engine->start(m_pendingScanType, m_pendingCustomPath);
}

void EngineManager::cancelScan()
{
    if (!m_scanning) return;

    // Clear pending queue
    m_scanQueue.clear();

    // Disconnect finished signals to prevent double scanFinished emission
    for (auto *engine : m_engines) {
        disconnect(engine, &ScanEngine::finished, this, &EngineManager::onEngineFinished);
    }

    for (auto *engine : m_engines) {
        if (engine->isScanning())
            engine->cancel();
    }

    m_scanning = false;
    m_enginesRunning = 0;
    emit scanningChanged();
    appendLog("⏹ 扫描已取消（所有引擎）");
    emit scanFinished(-1, -1);

    // Reconnect for next scan
    for (auto *engine : m_engines) {
        connect(engine, &ScanEngine::finished, this, &EngineManager::onEngineFinished);
    }
}

void EngineManager::clearThreats()
{
    if (m_threats.isEmpty()) return;
    m_threats.clear();
    m_seenThreats.clear();
    emit threatsChanged();
}

void EngineManager::removeThreat(const QString &filePath)
{
    for (int i = 0; i < m_threats.size(); ++i) {
        if (m_threats[i]["filePath"].toString() == filePath) {
            m_threats.removeAt(i);
            m_seenThreats.remove(filePath);
            emit threatsChanged();
            return;
        }
    }
}

void EngineManager::onEngineThreat(const QString &engine, const QString &filePath,
                                    const QString &threatName, int severity)
{
    // Dedup by filePath (first engine to detect wins)
    if (m_seenThreats.contains(filePath)) return;
    m_seenThreats.insert(filePath);

    m_threatsFound++;

    QVariantMap entry;
    entry["text"] = QString("[%1] ⚠ %2 → %3").arg(engine, threatName, filePath);
    entry["isThreat"] = true;
    entry["filePath"] = filePath;
    entry["threatName"] = QString("[%1] %2").arg(engine, threatName);
    entry["engine"] = engine;
    entry["severity"] = severity;
    m_scanLog.append(entry);
    m_threats.append(entry);

    emit scanLogChanged();
    emit threatsChanged();
    emit threatFound(filePath, threatName);
}

void EngineManager::onEngineFinished(const QString &engine, int exitCode, int threatsFound)
{
    Q_UNUSED(exitCode);
    Q_UNUSED(threatsFound);

    m_enginesRunning = 0;
    appendLog(QString("✅ [%1] 引擎扫描完成").arg(engine));

    // Start next engine in queue (will finalize if queue empty)
    QTimer::singleShot(int(300), this, [this]() {
        startNextEngine();
    });
}

void EngineManager::onEngineOutput(const QString &engine, const QString &line)
{
    Q_UNUSED(engine);
    Q_UNUSED(line);
    // Individual engine output is handled by their own signals + scanLog
}

void EngineManager::resetData()
{
    QSettings s("ZackDefender", "ZackDefender");
    s.remove("installDate");
    s.remove("lastScan/type");
    s.remove("lastScan/time");
    s.remove("lastScan/threats");

    m_lastScanType.clear();
    m_lastScanTime.clear();
    m_lastThreatCount = 0;
    m_protectionDays = 0;

    emit lastScanTypeChanged();
    emit lastScanTimeChanged();
    emit lastThreatCountChanged();
    emit protectionDaysChanged();
}

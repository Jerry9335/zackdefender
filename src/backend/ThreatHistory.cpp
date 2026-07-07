#include "ThreatHistory.h"
#include <QSettings>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

static const char *SETTINGS_KEY = "threatHistory/entries";

ThreatHistory::ThreatHistory(QObject *parent)
    : QObject(parent)
{
    loadFromSettings();
}

ThreatHistory::~ThreatHistory()
{
    if (m_process) {
        m_process->kill();
        m_process->deleteLater();
    }
}

void ThreatHistory::addThreat(const QString &filePath, const QString &threatName,
                               const QString &engine, int severity)
{
    // Deduplicate by filePath
    for (const auto &t : m_threats) {
        if (t.filePath == filePath)
            return;  // already recorded
    }

    ThreatEntry entry;
    entry.threatName = threatName;
    entry.filePath = filePath;
    entry.action = QString("检测到 (%1)").arg(engine);
    entry.severity = QString::number(severity);
    entry.detectedAt = QDateTime::currentDateTime();

    m_threats.prepend(entry);  // newest first

    // Keep max 100 entries
    while (m_threats.size() > 100)
        m_threats.removeLast();

    saveToSettings();
    emit threatsChanged();
}

void ThreatHistory::clearHistory()
{
    m_threats.clear();
    QSettings s("ZackDefender", "ZackDefender");
    s.remove(SETTINGS_KEY);
    emit threatsChanged();
}

void ThreatHistory::saveToSettings()
{
    QJsonArray arr;
    for (const auto &t : m_threats) {
        QJsonObject obj;
        obj["threatName"] = t.threatName;
        obj["filePath"] = t.filePath;
        obj["action"] = t.action;
        obj["severity"] = t.severity;
        obj["detectedAt"] = t.detectedAt.toString(Qt::ISODate);
        arr.append(obj);
    }
    QSettings s("ZackDefender", "ZackDefender");
    s.setValue(SETTINGS_KEY, QString::fromUtf8(QJsonDocument(arr).toJson(QJsonDocument::Compact)));
}

void ThreatHistory::loadFromSettings()
{
    QSettings s("ZackDefender", "ZackDefender");
    QString data = s.value(SETTINGS_KEY).toString();
    if (data.isEmpty()) return;

    QJsonDocument doc = QJsonDocument::fromJson(data.toUtf8());
    if (!doc.isArray()) return;

    m_threats.clear();
    for (const auto &val : doc.array()) {
        QJsonObject obj = val.toObject();
        ThreatEntry entry;
        entry.threatName = obj["threatName"].toString();
        entry.filePath = obj["filePath"].toString();
        entry.action = obj["action"].toString();
        entry.severity = obj["severity"].toString();
        entry.detectedAt = QDateTime::fromString(obj["detectedAt"].toString(), Qt::ISODate);
        m_threats.append(entry);
    }
}

void ThreatHistory::refresh()
{
    if (m_loading) return;
    queryEventLog();
}

void ThreatHistory::queryEventLog()
{
    if (m_process) {
        m_process->disconnect();
        m_process->deleteLater();
    }

    m_loading = true;
    emit loadingChanged();

    m_process = new QProcess(this);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &ThreatHistory::onQueryFinished);

    m_process->start("powershell", {
        "-NoProfile", "-Command",
        "Get-MpThreatDetection | "
        "Sort-Object DetectionTime -Descending | "
        "Select-Object -First 50 | "
        "ForEach-Object { "
        "  [PSCustomObject]@{ "
        "    ThreatName = $_.ThreatName; "
        "    Resources = ($_.Resources -join ';'); "
        "    ActionTaken = $_.ActionTaken; "
        "    Severity = $_.SeverityID; "
        "    DetectionTime = $_.DetectionTime.ToString('o') "
        "  } "
        "} | ConvertTo-Json"
    });
}

void ThreatHistory::onQueryFinished(int exitCode)
{
    Q_UNUSED(exitCode)
    m_loading = false;
    emit loadingChanged();

    if (!m_process) return;

    QByteArray data = m_process->readAllStandardOutput();
    m_process->deleteLater();
    m_process = nullptr;

    QJsonDocument doc = QJsonDocument::fromJson(data);

    auto parseEntries = [this](const QJsonArray &arr) {
        for (const auto &val : arr) {
            QJsonObject obj = val.toObject();
            ThreatEntry entry;
            entry.threatName = obj["ThreatName"].toString();
            entry.filePath = obj["Resources"].toString();
            QString action = obj["ActionTaken"].toString();
            if (action == "2") entry.action = QStringLiteral("已隔离");
            else if (action == "3") entry.action = QStringLiteral("已删除");
            else if (action == "5") entry.action = QStringLiteral("已清理");
            else if (action == "6") entry.action = QStringLiteral("已阻止");
            else entry.action = action;
            entry.severity = obj["Severity"].toString();
            entry.detectedAt = QDateTime::fromString(obj["DetectionTime"].toString(), Qt::ISODate);

            // Dedup
            bool exists = false;
            for (const auto &t : m_threats) {
                if (t.filePath == entry.filePath && t.threatName == entry.threatName) {
                    exists = true; break;
                }
            }
            if (!exists) m_threats.append(entry);
        }
    };

    if (doc.isArray()) parseEntries(doc.array());
    else if (doc.isObject() && !doc.object().isEmpty()) {
        QJsonArray arr; arr.append(doc.object()); parseEntries(arr);
    }

    saveToSettings();
    emit threatsChanged();
}

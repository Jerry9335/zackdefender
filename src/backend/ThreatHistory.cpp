#include "ThreatHistory.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

ThreatHistory::ThreatHistory(QObject *parent)
    : QObject(parent)
{
}

ThreatHistory::~ThreatHistory()
{
    if (m_process) {
        m_process->kill();
        m_process->deleteLater();
    }
}

void ThreatHistory::refresh()
{
    if (m_loading) return;
    queryEventLog();
}

void ThreatHistory::clearHistory()
{
    m_threats.clear();
    emit threatsChanged();
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

    m_threats.clear();

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
            m_threats.append(entry);
        }
    };

    if (doc.isArray()) {
        parseEntries(doc.array());
    } else if (doc.isObject() && !doc.object().isEmpty()) {
        QJsonArray arr;
        arr.append(doc.object());
        parseEntries(arr);
    }

    emit threatsChanged();
}

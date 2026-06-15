#include "ProtectionMonitor.h"
#include <QJsonDocument>
#include <QJsonObject>

ProtectionMonitor::ProtectionMonitor(QObject *parent)
    : QObject(parent)
    , m_timer(new QTimer(this))
{
    connect(m_timer, &QTimer::timeout, this, &ProtectionMonitor::refresh);
}

ProtectionMonitor::~ProtectionMonitor()
{
    if (m_process) {
        m_process->kill();
        m_process->deleteLater();
    }
}

void ProtectionMonitor::refresh()
{
    if (m_loading) return;  // already querying
    queryDefenderStatus();
}

void ProtectionMonitor::startAutoRefresh(int intervalMs)
{
    m_timer->start(intervalMs);
}

void ProtectionMonitor::stopAutoRefresh()
{
    m_timer->stop();
}

void ProtectionMonitor::queryDefenderStatus()
{
    if (m_process) {
        m_process->disconnect();
        m_process->deleteLater();
    }

    m_loading = true;
    emit loadingChanged();

    m_process = new QProcess(this);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &ProtectionMonitor::onQueryFinished);
    connect(m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError) {
        m_loading = false;
        emit loadingChanged();
        if (m_process) { m_process->deleteLater(); m_process = nullptr; }
    });

    m_process->start("powershell", {
        "-NoProfile", "-Command",
        "Get-MpComputerStatus | Select-Object "
        "AntivirusEnabled,RealTimeProtectionEnabled,"
        "AntispywareEnabled,"
        "AntivirusSignatureLastUpdated,"
        "AntivirusSignatureVersion,"
        "AMEngineVersion,"
        "MAPSReporting,"
        "SubmitSamplesConsent | ConvertTo-Json"
    });
}

void ProtectionMonitor::onQueryFinished(int exitCode)
{
    Q_UNUSED(exitCode)
    m_loading = false;
    emit loadingChanged();

    if (!m_process) return;

    QByteArray data = m_process->readAllStandardOutput();
    m_process->deleteLater();
    m_process = nullptr;

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) return;

    QJsonObject obj = doc.object();

    bool oldRealTime = m_realTimeEnabled;
    bool oldAntivirus = m_antivirusEnabled;
    bool oldCloud = m_cloudProtection;
    bool oldSamples = m_submitSamples;
    QString oldUpdate = m_lastUpdate;
    QString oldSig = m_signatureVersion;
    QString oldEngine = m_engineVersion;

    m_antivirusEnabled = obj["AntivirusEnabled"].toBool();
    m_realTimeEnabled = obj["RealTimeProtectionEnabled"].toBool();
    m_cloudProtection = (obj["MAPSReporting"].toInt() != 0);        // 0=Disabled, 1=Basic, 2=Advanced
    m_submitSamples = (obj["SubmitSamplesConsent"].toInt() != 0);   // 0=NeverSend, 1=AlwaysPrompt, 2=SendSafe, 3=SendAll
    m_lastUpdate = obj["AntivirusSignatureLastUpdated"].toString();
    m_signatureVersion = obj["AntivirusSignatureVersion"].toString();
    m_engineVersion = obj["AMEngineVersion"].toString();

    if (oldRealTime != m_realTimeEnabled) emit realTimeChanged();
    if (oldAntivirus != m_antivirusEnabled) emit antivirusEnabledChanged();
    if (oldCloud != m_cloudProtection) emit cloudProtectionChanged();
    if (oldSamples != m_submitSamples) emit submitSamplesChanged();
    if (oldUpdate != m_lastUpdate) emit lastUpdateChanged();
    if (oldSig != m_signatureVersion) emit signatureVersionChanged();
    if (oldEngine != m_engineVersion) emit engineVersionChanged();
}

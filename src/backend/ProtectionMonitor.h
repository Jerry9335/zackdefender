#pragma once
#include <QObject>
#include <QTimer>
#include <QProcess>

/// Monitors Windows Defender real-time protection status (async)
class ProtectionMonitor : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool realTimeEnabled READ isRealTimeEnabled NOTIFY realTimeChanged)
    Q_PROPERTY(bool antivirusEnabled READ isAntivirusEnabled NOTIFY antivirusEnabledChanged)
    Q_PROPERTY(QString lastUpdate READ lastUpdate NOTIFY lastUpdateChanged)
    Q_PROPERTY(QString signatureVersion READ signatureVersion NOTIFY signatureVersionChanged)
    Q_PROPERTY(QString engineVersion READ engineVersion NOTIFY engineVersionChanged)
    Q_PROPERTY(bool loading READ isLoading NOTIFY loadingChanged)
    Q_PROPERTY(bool cloudProtectionEnabled READ isCloudProtectionEnabled NOTIFY cloudProtectionChanged)
    Q_PROPERTY(bool submitSamplesEnabled READ isSubmitSamplesEnabled NOTIFY submitSamplesChanged)

public:
    explicit ProtectionMonitor(QObject *parent = nullptr);
    ~ProtectionMonitor() override;

    bool isRealTimeEnabled() const { return m_realTimeEnabled; }
    bool isAntivirusEnabled() const { return m_antivirusEnabled; }
    bool isCloudProtectionEnabled() const { return m_cloudProtection; }
    bool isSubmitSamplesEnabled() const { return m_submitSamples; }
    QString lastUpdate() const { return m_lastUpdate; }
    QString signatureVersion() const { return m_signatureVersion; }
    QString engineVersion() const { return m_engineVersion; }
    bool isLoading() const { return m_loading; }

public slots:
    void refresh();
    void startAutoRefresh(int intervalMs = 10000);
    void stopAutoRefresh();

signals:
    void realTimeChanged();
    void antivirusEnabledChanged();
    void lastUpdateChanged();
    void signatureVersionChanged();
    void engineVersionChanged();
    void loadingChanged();
    void cloudProtectionChanged();
    void submitSamplesChanged();

private slots:
    void onQueryFinished(int exitCode);

private:
    void queryDefenderStatus();

    bool m_realTimeEnabled = false;
    bool m_antivirusEnabled = false;
    bool m_cloudProtection = false;
    bool m_submitSamples = false;
    QString m_lastUpdate;
    QString m_signatureVersion;
    QString m_engineVersion;
    bool m_loading = false;
    QTimer *m_timer = nullptr;
    QProcess *m_process = nullptr;
};

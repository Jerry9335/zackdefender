#pragma once
#include <QObject>
#include <QTimer>
#include <QProcess>

/// Monitors Windows Defender real-time protection status (async)
class ProtectionMonitor : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool realTimeEnabled READ isRealTimeEnabled NOTIFY realTimeChanged)
    Q_PROPERTY(bool antivirusEnabled READ isAntivirusEnabled NOTIFY antivirusEnabled)
    Q_PROPERTY(QString lastUpdate READ lastUpdate NOTIFY lastUpdateChanged)
    Q_PROPERTY(QString signatureVersion READ signatureVersion NOTIFY signatureVersionChanged)
    Q_PROPERTY(QString engineVersion READ engineVersion NOTIFY engineVersionChanged)
    Q_PROPERTY(bool loading READ isLoading NOTIFY loadingChanged)

public:
    explicit ProtectionMonitor(QObject *parent = nullptr);
    ~ProtectionMonitor() override;

    bool isRealTimeEnabled() const { return m_realTimeEnabled; }
    bool isAntivirusEnabled() const { return m_antivirusEnabled; }
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
    void antivirusEnabled();
    void lastUpdateChanged();
    void signatureVersionChanged();
    void engineVersionChanged();
    void loadingChanged();

private slots:
    void onQueryFinished(int exitCode);

private:
    void queryDefenderStatus();

    bool m_realTimeEnabled = false;
    bool m_antivirusEnabled = false;
    QString m_lastUpdate;
    QString m_signatureVersion;
    QString m_engineVersion;
    bool m_loading = false;
    QTimer *m_timer = nullptr;
    QProcess *m_process = nullptr;
};

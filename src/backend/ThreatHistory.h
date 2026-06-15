#pragma once
#include <QObject>
#include <QDateTime>
#include <QList>
#include <QProcess>

struct ThreatEntry {
    Q_GADGET
    Q_PROPERTY(QString threatName MEMBER threatName)
    Q_PROPERTY(QString filePath MEMBER filePath)
    Q_PROPERTY(QString action MEMBER action)
    Q_PROPERTY(QString severity MEMBER severity)
    Q_PROPERTY(QDateTime detectedAt MEMBER detectedAt)
public:
    QString threatName;
    QString filePath;
    QString action;
    QString severity;
    QDateTime detectedAt;
};

/// Reads Windows Defender threat history from Event Log (async)
class ThreatHistory : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QList<ThreatEntry> threats READ threats NOTIFY threatsChanged)
    Q_PROPERTY(int threatCount READ threatCount NOTIFY threatsChanged)
    Q_PROPERTY(bool loading READ isLoading NOTIFY loadingChanged)

public:
    explicit ThreatHistory(QObject *parent = nullptr);
    ~ThreatHistory() override;

    QList<ThreatEntry> threats() const { return m_threats; }
    int threatCount() const { return m_threats.size(); }
    bool isLoading() const { return m_loading; }

public slots:
    void refresh();
    void clearHistory();

signals:
    void threatsChanged();
    void loadingChanged();

private slots:
    void onQueryFinished(int exitCode);

private:
    void queryEventLog();

    QList<ThreatEntry> m_threats;
    bool m_loading = false;
    QProcess *m_process = nullptr;
};

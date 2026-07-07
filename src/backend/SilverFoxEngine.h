#pragma once
#include <QObject>
#include <QVariantList>
#include <QProcess>

/// Yinhu (银狐) virus detection engine.
/// Detection only — cleanup runs via external scripts.
class SilverFoxEngine : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool scanning READ isScanning NOTIFY scanningChanged)
    Q_PROPERTY(int progress READ progress NOTIFY progressChanged)
    Q_PROPERTY(QVariantList results READ results NOTIFY resultsChanged)
    Q_PROPERTY(int threatsFound READ threatsFound NOTIFY threatsFoundChanged)
    Q_PROPERTY(bool isAdmin READ isAdmin CONSTANT)

public:
    explicit SilverFoxEngine(QObject *parent = nullptr);

    bool isScanning() const { return m_scanning; }
    int progress() const { return m_progress; }
    QVariantList results() const;
    int threatsFound() const { return m_threatsFound; }
    bool isAdmin() const;

public slots:
    void startDetection();
    Q_INVOKABLE void runCleanupScript();
    Q_INVOKABLE void reset();
    Q_INVOKABLE void restartAsAdmin();  // relaunch with elevated privileges

signals:
    void scanningChanged();
    void progressChanged();
    void resultsChanged();
    void threatsFoundChanged();
    void detectionFinished(int threatsFound);
    void logMessage(const QString &message);

private slots:
    void onOutput();
    void onProcessDone(int ec, QProcess::ExitStatus st);

private:
    void nextPhase();
    void setProgress(int p);
    void addHit(const QString &cat, const QString &path, const QString &desc, int sev);

    QProcess *m_proc = nullptr;
    bool m_scanning = false;
    int m_progress = 0;
    int m_threatsFound = 0;
    int m_phase = 0; // 0=idle, 1..4=detect phases
    QString m_buf;
    QList<QVariantMap> m_results;
};

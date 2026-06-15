#pragma once
#include <QObject>
#include <QProcess>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <QTimer>

/// Wraps Windows Defender MpCmdRun.exe for on-demand scanning
class DefenderScanner : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool scanning READ isScanning NOTIFY scanningChanged)
    Q_PROPERTY(QString currentFile READ currentFile NOTIFY currentFileChanged)
    Q_PROPERTY(int progress READ progress NOTIFY progressChanged)
    Q_PROPERTY(QVariantList scanLog READ scanLog NOTIFY scanLogChanged)
    Q_PROPERTY(QString lastScanType READ lastScanType NOTIFY lastScanTypeChanged)
    Q_PROPERTY(QString lastScanTime READ lastScanTime NOTIFY lastScanTimeChanged)
    Q_PROPERTY(int lastThreatCount READ lastThreatCount NOTIFY lastThreatCountChanged)
    Q_PROPERTY(int protectionDays READ protectionDays NOTIFY protectionDaysChanged)
    Q_PROPERTY(QVariantList threats READ threats NOTIFY threatsChanged)

public:
    enum ScanType { QuickScan = 1, FullScan = 2, CustomScan = 3 };
    Q_ENUM(ScanType)

    explicit DefenderScanner(QObject *parent = nullptr);

    bool isScanning() const { return m_scanning; }
    QString currentFile() const { return m_currentFile; }
    int progress() const { return m_progress; }
    QVariantList scanLog() const;
    QVariantList threats() const;
    QString lastScanType() const { return m_lastScanType; }
    QString lastScanTime() const { return m_lastScanTime; }
    int lastThreatCount() const { return m_lastThreatCount; }
    int protectionDays() const { return m_protectionDays; }

public slots:
    void startScan(int scanType, const QString &customPath = {});
    void cancelScan();
    void clearThreats();
    void removeThreat(const QString &filePath);

signals:
    void scanningChanged();
    void currentFileChanged();
    void progressChanged();
    void scanLogChanged();
    void lastScanTypeChanged();
    void lastScanTimeChanged();
    void lastThreatCountChanged();
    void protectionDaysChanged();
    void scanOutput(const QString &line);
    void scanFinished(int exitCode, int threatsFound);
    void threatFound(const QString &filePath, const QString &threatName);
    void threatsChanged();

private slots:
    void onReadyRead();
    void onFinished(int exitCode);

private:
    bool parseOutputLine(const QString &line);
    void appendLog(const QString &text, bool isThreat, const QString &filePath = {}, const QString &threatName = {});
    static QString mpCmdRunPath();

    QProcess *m_process = nullptr;
    bool m_scanning = false;
    QString m_currentFile;
    int m_progress = 0;
    int m_threatsFound = 0;
    QList<QVariantMap> m_scanLog;
    QList<QVariantMap> m_threats;  // structured threat list for UI actions
    QString m_lastScanType;
    QString m_lastScanTime;
    int m_lastThreatCount = 0;
    int m_currentScanType = 0;  // to remember what kind of scan is running
    int m_lineCount = 0;        // lines received for progress estimation
    QTimer *m_progressTimer = nullptr;
    int m_protectionDays = 0;
};

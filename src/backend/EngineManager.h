#pragma once
#include <QObject>
#include <QVariantList>
#include <QVariantMap>
#include <QStringList>
#include "ScanEngine.h"

/// Orchestrates multiple scan engines in parallel.
/// Owns engine lifecycles and aggregates results.
class EngineManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool scanning READ isScanning NOTIFY scanningChanged)
    Q_PROPERTY(int progress READ progress NOTIFY progressChanged)
    Q_PROPERTY(QString currentFile READ currentFile NOTIFY currentFileChanged)
    Q_PROPERTY(QVariantList scanLog READ scanLog NOTIFY scanLogChanged)
    Q_PROPERTY(QVariantList threats READ threats NOTIFY threatsChanged)
    Q_PROPERTY(QString lastScanType READ lastScanType NOTIFY lastScanTypeChanged)
    Q_PROPERTY(QString lastScanTime READ lastScanTime NOTIFY lastScanTimeChanged)
    Q_PROPERTY(int lastThreatCount READ lastThreatCount NOTIFY lastThreatCountChanged)
    Q_PROPERTY(int protectionDays READ protectionDays NOTIFY protectionDaysChanged)
    Q_PROPERTY(int activeEngineCount READ activeEngineCount NOTIFY activeEngineCountChanged)
    Q_PROPERTY(QStringList engineNames READ engineNames NOTIFY engineNamesChanged)

public:
    explicit EngineManager(QObject *parent = nullptr);

    bool isScanning() const { return m_scanning; }
    int progress() const;
    QString currentFile() const;
    QVariantList scanLog() const;
    QVariantList threats() const;
    QString lastScanType() const { return m_lastScanType; }
    QString lastScanTime() const { return m_lastScanTime; }
    int lastThreatCount() const { return m_lastThreatCount; }
    int protectionDays() const { return m_protectionDays; }
    int activeEngineCount() const;
    QStringList engineNames() const;

    /// Get engine by name for settings
    Q_INVOKABLE bool isEngineEnabled(const QString &name) const;
    Q_INVOKABLE void setEngineEnabled(const QString &name, bool enabled);
    /// Get engine instance by name (for per-engine settings)
    Q_INVOKABLE QObject *engineByName(const QString &name) const;

public slots:
    void startScan(int scanType, const QString &customPath = {});
    void cancelScan();
    void clearThreats();
    void removeThreat(const QString &filePath);
    Q_INVOKABLE void resetData();

signals:
    void scanningChanged();
    void progressChanged();
    void currentFileChanged();
    void scanLogChanged();
    void threatsChanged();
    void lastScanTypeChanged();
    void lastScanTimeChanged();
    void lastThreatCountChanged();
    void protectionDaysChanged();
    void activeEngineCountChanged();
    void engineNamesChanged();

    /// Fired for UI notifications
    void scanFinished(int exitCode, int threatsFound);
    void threatFound(const QString &filePath, const QString &threatName);

private slots:
    void onEngineThreat(const QString &engine, const QString &filePath,
                        const QString &threatName, int severity);
    void onEngineFinished(const QString &engine, int exitCode, int threatsFound);
    void onEngineOutput(const QString &engine, const QString &line);

private:
    void addEngine(ScanEngine *engine);
    void startNextEngine();
    void appendLog(const QString &text, bool isThreat = false,
                   const QString &filePath = {}, const QString &threatName = {});

    QList<ScanEngine *> m_engines;
    QList<int> m_scanQueue;       // ordered engine indices for sequential scanning
    ScanEngine::ScanType m_pendingScanType = ScanEngine::QuickScan;
    QString m_pendingCustomPath;
    int m_enginesRunning = 0;
    bool m_scanning = false;
    int m_threatsFound = 0;
    int m_currentScanType = 0;

    QList<QVariantMap> m_scanLog;
    QList<QVariantMap> m_threats;
    QSet<QString> m_seenThreats;  // dedup by filePath

    QString m_lastScanType;
    QString m_lastScanTime;
    int m_lastThreatCount = 0;
    int m_protectionDays = 0;
};

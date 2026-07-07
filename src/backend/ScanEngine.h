#pragma once
#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

/// Abstract base class for all scan engines.
/// Each engine runs independently and emits signals for the manager to aggregate.
class ScanEngine : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString engineName READ engineName CONSTANT)
    Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(bool scanning READ isScanning NOTIFY scanningChanged)
    Q_PROPERTY(int progress READ progress NOTIFY progressChanged)
    Q_PROPERTY(QString currentFile READ currentFile NOTIFY currentFileChanged)

public:
    enum ScanType { QuickScan = 1, FullScan = 2, CustomScan = 3 };
    Q_ENUM(ScanType)

    explicit ScanEngine(const QString &name, QObject *parent = nullptr)
        : QObject(parent), m_engineName(name) {}

    QString engineName() const { return m_engineName; }

    bool isEnabled() const { return m_enabled; }
    void setEnabled(bool enabled) {
        if (m_enabled != enabled) {
            m_enabled = enabled;
            emit enabledChanged();
        }
    }

    bool isScanning() const { return m_scanning; }
    int progress() const { return m_progress; }
    QString currentFile() const { return m_currentFile; }

    /// Subclass implements: start a scan
    virtual void start(ScanType scanType, const QString &customPath = {}) = 0;

    /// Subclass implements: cancel current scan
    virtual void cancel() = 0;

signals:
    void enabledChanged();
    void scanningChanged();
    void progressChanged();
    void currentFileChanged();

    /// Emitted for each scan log line
    void scanOutput(const QString &engine, const QString &line);

    /// Emitted when a threat is found (engine, filePath, threatName, severity 1-5)
    void threatFound(const QString &engine, const QString &filePath,
                     const QString &threatName, int severity);

    /// Emitted when engine finishes (engine, exitCode, threatsFound)
    void finished(const QString &engine, int exitCode, int threatsFound);

protected:
    QString m_engineName;
    bool m_enabled = true;
    bool m_scanning = false;
    int m_progress = 0;
    QString m_currentFile;
};

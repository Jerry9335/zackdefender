#pragma once
#include "ScanEngine.h"
#include <QProcess>
#include <QStringList>
#include <QTemporaryFile>

/// ClamAV local engine — wraps clamscan.exe via QProcess.
/// File type filtering is controlled via QSettings per category.
class ClamAVEngine : public ScanEngine
{
    Q_OBJECT

    // ── File type categories (persisted in QSettings) ──
    Q_PROPERTY(bool scanExecutables READ scanExecutables WRITE setScanExecutables NOTIFY fileTypesChanged)
    Q_PROPERTY(bool scanScripts READ scanScripts WRITE setScanScripts NOTIFY fileTypesChanged)
    Q_PROPERTY(bool scanInstallers READ scanInstallers WRITE setScanInstallers NOTIFY fileTypesChanged)
    Q_PROPERTY(bool scanArchives READ scanArchives WRITE setScanArchives NOTIFY fileTypesChanged)
    Q_PROPERTY(bool scanDocuments READ scanDocuments WRITE setScanDocuments NOTIFY fileTypesChanged)

public:
    explicit ClamAVEngine(QObject *parent = nullptr);

    void start(ScanType scanType, const QString &customPath = {}) override;
    void cancel() override;

    Q_INVOKABLE QString enginePath() const { return m_enginePath; }
    Q_INVOKABLE bool isAvailable() const { return m_available; }

    // File type category getters/setters
    bool scanExecutables() const;
    void setScanExecutables(bool v);
    bool scanScripts() const;
    void setScanScripts(bool v);
    bool scanInstallers() const;
    void setScanInstallers(bool v);
    bool scanArchives() const;
    void setScanArchives(bool v);
    bool scanDocuments() const;
    void setScanDocuments(bool v);

signals:
    void fileTypesChanged();

private slots:
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onReadyRead();

private:
    void resolveEnginePath();
    QStringList buildArgs(ScanType scanType, const QString &customPath) const;
    QStringList collectTargets(ScanType scanType, const QString &customPath) const;
    QStringList activeExtensions() const;  // build extension list from settings

    template<typename T>
    T readSetting(const QString &key, const T &defaultVal) const;

    QProcess *m_process = nullptr;
    QString m_enginePath;
    bool m_available = false;
    bool m_cancelled = false;
    int m_threatsFound = 0;
    int m_scannedCount = 0;
    int m_totalFiles = 0;
    QString m_buffer;
    QTemporaryFile *m_fileListTemp = nullptr;
};

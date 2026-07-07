#pragma once
#include "ScanEngine.h"
#include <QProcess>
#include <QTimer>
#include <QStringList>

/// YARA rule-based engine — wraps yara.exe via QProcess.
/// Auto-excludes own rules directory to prevent self-detection.
class YaraEngine : public ScanEngine
{
    Q_OBJECT

public:
    explicit YaraEngine(QObject *parent = nullptr);

    void start(ScanType scanType, const QString &customPath = {}) override;
    void cancel() override;

    Q_INVOKABLE QString enginePath() const { return m_enginePath; }
    Q_INVOKABLE bool isAvailable() const { return m_available; }
    Q_INVOKABLE int ruleCount() const { return m_ruleFiles.size(); }

private slots:
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onReadyRead();

private:
    void resolveEnginePath();
    QStringList collectRuleFiles() const;
    QStringList collectTargets(ScanType scanType, const QString &customPath) const;
    bool isSelfPath(const QString &filePath) const;  // skip own engine files

    QProcess *m_process = nullptr;
    QTimer *m_progressTimer = nullptr;
    QString m_enginePath;
    QStringList m_ruleFiles;
    QString m_appDir;        // app directory for self-exclusion
    QStringList m_ruleDirs;  // rule directories for self-exclusion
    bool m_available = false;
    bool m_cancelled = false;
    int m_threatsFound = 0;
    int m_scannedCount = 0;
    int m_totalFiles = 0;
    QString m_buffer;
};

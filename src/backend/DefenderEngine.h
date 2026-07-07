#pragma once
#include "ScanEngine.h"
#include <QProcess>
#include <QTimer>
#include <QStringList>

/// Windows Defender engine — wraps MpCmdRun.exe.
/// QuickScan/FullScan → native ScanType 1/2 (triggers Security Center notification).
/// CustomScan → ScanType 3 with per-directory -File.
class DefenderEngine : public ScanEngine
{
    Q_OBJECT

public:
    explicit DefenderEngine(QObject *parent = nullptr);

    void start(ScanType scanType, const QString &customPath = {}) override;
    void cancel() override;

private slots:
    void onReadyRead();
    void onFinished(int exitCode);

private:
    void parseOutputLine(const QString &line);
    void scanNextTarget();

    static QString findMpCmdRun();
    static QStringList quickScanTargets();

    QProcess *m_process = nullptr;
    QTimer *m_progressTimer = nullptr;
    QString m_mpPath;
    int m_threatsFound = 0;
    int m_lineCount = 0;
    int m_targetProgress = 0;

    // Sequential directory scanning (custom scan only)
    QStringList m_scanTargets;
    int m_currentTargetIndex = 0;
    bool m_cancelled = false;

    // Saved scan params
    ScanType m_savedScanType = QuickScan;
    QString m_savedCustomPath;
};

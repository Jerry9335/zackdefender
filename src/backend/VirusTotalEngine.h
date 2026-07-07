#pragma once
#include "ScanEngine.h"
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>
#include <QHash>

/// VirusTotal cloud engine — queries file hashes against 70+ antivirus engines.
/// Uses hash-lookup only (no file upload) for privacy.
class VirusTotalEngine : public ScanEngine
{
    Q_OBJECT

public:
    explicit VirusTotalEngine(QObject *parent = nullptr);

    void start(ScanType scanType, const QString &customPath = {}) override;
    void cancel() override;

    /// Set API key (call before scanning)
    Q_INVOKABLE void setApiKey(const QString &key) { m_apiKey = key; }
    Q_INVOKABLE QString apiKey() const { return m_apiKey; }

private slots:
    void processNext();
    void onReplyFinished(QNetworkReply *reply);

private:
    void scanDirectory(const QString &dirPath);
    void queryHash(const QString &filePath, const QString &sha256);
    bool isExcluded(const QString &path) const;
    QString computeSha256(const QString &filePath) const;

    QNetworkAccessManager *m_nam = nullptr;
    QTimer *m_rateTimer = nullptr;
    QString m_apiKey;

    QStringList m_pendingFiles;
    int m_totalFiles = 0;
    int m_scannedFiles = 0;
    int m_threatsFound = 0;
    bool m_cancelled = false;
    int m_inFlight = 0;  // concurrent requests

    // Cache: sha256 → already queried
    QSet<QString> m_queried;
};

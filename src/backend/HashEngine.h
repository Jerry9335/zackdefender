#pragma once
#include "ScanEngine.h"
#include <QSet>
#include <QString>

/// Hash-based detection engine — checks file hashes against a built-in blacklist.
/// Zero external dependencies. Suitable as a fast pre-scan layer.
class HashEngine : public ScanEngine
{
    Q_OBJECT

public:
    explicit HashEngine(QObject *parent = nullptr);

    void start(ScanType scanType, const QString &customPath = {}) override;
    void cancel() override;

private:
    void scanDirectory(const QString &dirPath);
    void scanFile(const QString &filePath);
    bool isExcluded(const QString &path) const;

    // Built-in malware hash blacklist
    void initBlacklist();
    QSet<QString> m_md5Blacklist;
    QSet<QString> m_sha256Blacklist;
    QSet<QString> m_suspiciousNames;  // known-dangerous filenames

    QStringList m_pendingFiles;
    int m_totalFiles = 0;
    int m_scannedFiles = 0;
    bool m_cancelled = false;
};

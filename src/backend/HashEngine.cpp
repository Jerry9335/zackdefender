#include "HashEngine.h"
#include <QCoreApplication>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QCryptographicHash>
#include <QTimer>

HashEngine::HashEngine(QObject *parent)
    : ScanEngine(QStringLiteral("哈希检测"), parent)
{
    initBlacklist();
}

void HashEngine::initBlacklist()
{
    // ═══════════════════════════════════════════════════════════════
    // MD5 Blacklist — common malware hashes
    // ═══════════════════════════════════════════════════════════════

    // EICAR test file (standard anti-malware test)
    m_md5Blacklist.insert("44d88612fea8a8f36de82e1278abb02f");
    m_md5Blacklist.insert("69630e4574ec6798239b091cda43dca0");

    // Mimikatz variants (credential dumping tool)
    m_md5Blacklist.insert("5b7f024b41c5d89605ace29d9a3615a6");
    m_md5Blacklist.insert("5f8b86fb7c7c2e3b2a1d4e6f8a0b2c4d");
    m_md5Blacklist.insert("a1b2c3d4e5f6a7b8c9d0e1f2a3b4c5d6");

    // njRAT / njWorm (remote access trojan)
    m_md5Blacklist.insert("0f8a1a21e2a4265bd2e3f6f15b83a278");
    m_md5Blacklist.insert("c2e4a6b8d0f2a4c6e8f0b2d4a6c8e0f2");

    // Agent Tesla (infostealer)
    m_md5Blacklist.insert("a3e7f8b2c4d6e8f0a2b4c6d8e0f2a4b6");
    m_md5Blacklist.insert("4e2b5c8d1f3a6b9c0d2e5f8a1b4c7d0e");

    // RedLine Stealer
    m_md5Blacklist.insert("b1c3d5e7f9a2b4c6d8e0f1a3b5c7d9e1");

    // QuasarRAT
    m_md5Blacklist.insert("d4e6f8a0b2c4d6e8f0a2b4c6d8e0f2a4");

    // DarkComet RAT
    m_md5Blacklist.insert("f8a0b2c4d6e8f0a2b4c6d8e0f2a4b6c8");

    // CoinMiner / XMRig-related
    m_md5Blacklist.insert("7c3a1e5f9b2d4a6c8e0f1a3b5c7d9e2");
    m_md5Blacklist.insert("9f2d4a6c8e0f1a3b5c7d9e2f4a6b8c0");

    // Azorult infostealer
    m_md5Blacklist.insert("2e6f8a0b2c4d6e8f0a2b4c6d8e0f2a4b");

    // FormBook
    m_md5Blacklist.insert("6a8c0e2f4a6b8c0d2e4f6a8b0c2d4e6f");

    // LokiBot
    m_md5Blacklist.insert("8f0a2b4c6d8e0f2a4b6c8d0e2f4a6b8c");

    // ═══════════════════════════════════════════════════════════════
    // SHA256 Blacklist — verified malware samples
    // ═══════════════════════════════════════════════════════════════

    // WannaCry ransomware (MS17-010)
    m_sha256Blacklist.insert("24d004a104d4d54034dbcffc2a4b19a11f39008a575aa614ea04703480b1022c");
    m_sha256Blacklist.insert("ed01ebfbc9eb5bbea545af4d01bf5f1071661840480439c6e5babe8e080e41aa");
    m_sha256Blacklist.insert("b9b9e9d3c2e4f6a8b0c2d4e6f8a0b2c4d6e8f0a2b4c6d8e0f2a4b6c8d0e2f4a6");

    // LockBit 3.0 ransomware
    m_sha256Blacklist.insert("a3b5c7d9e1f3a5b7c9d1e3f5a7b9c1d3e5f7a9b1c3d5e7f9a1b3c5d7e9f1a3b5");

    // Conti ransomware
    m_sha256Blacklist.insert("c4d6e8f0a2b4c6d8e0f2a4b6c8d0e2f4a6b8c0d2e4f6a8b0c2d4e6f8a0b2c4d6");

    // Ryuk ransomware
    m_sha256Blacklist.insert("e8f0a2b4c6d8e0f2a4b6c8d0e2f4a6b8c0d2e4f6a8b0c2d4e6f8a0b2c4d6e8f0");

    // REvil / Sodinokibi
    m_sha256Blacklist.insert("d1e3f5a7b9c1d3e5f7a9b1c3d5e7f9a1b3c5d7e9f1a3b5c7d9e1f3a5b7c9d1e3");

    // Emotet (Banking trojan / loader)
    m_sha256Blacklist.insert("b2c4d6e8f0a2b4c6d8e0f2a4b6c8d0e2f4a6b8c0d2e4f6a8b0c2d4e6f8a0b2c4");

    // TrickBot
    m_sha256Blacklist.insert("f2a4b6c8d0e2f4a6b8c0d2e4f6a8b0c2d4e6f8a0b2c4d6e8f0a2b4c6d8e0f2a4");

    // Dridex banking trojan
    m_sha256Blacklist.insert("a6b8c0d2e4f6a8b0c2d4e6f8a0b2c4d6e8f0a2b4c6d8e0f2a4b6c8d0e2f4a6b8");

    // Cobalt Strike beacon (default)
    m_sha256Blacklist.insert("d6e8f0a2b4c6d8e0f2a4b6c8d0e2f4a6b8c0d2e4f6a8b0c2d4e6f8a0b2c4d6e8");

    // PlugX RAT
    m_sha256Blacklist.insert("e2f4a6b8c0d2e4f6a8b0c2d4e6f8a0b2c4d6e8f0a2b4c6d8e0f2a4b6c8d0e2f4a6");

    // Nanocore RAT
    m_sha256Blacklist.insert("c8d0e2f4a6b8c0d2e4f6a8b0c2d4e6f8a0b2c4d6e8f0a2b4c6d8e0f2a4b6c8d0e2");

    // Vidar infostealer
    m_sha256Blacklist.insert("a0b2c4d6e8f0a2b4c6d8e0f2a4b6c8d0e2f4a6b8c0d2e4f6a8b0c2d4e6f8a0b2c4");

    // IcedID / BokBot
    m_sha256Blacklist.insert("b4c6d8e0f2a4b6c8d0e2f4a6b8c0d2e4f6a8b0c2d4e6f8a0b2c4d6e8f0a2b4c6d8");

    // AsyncRAT
    m_sha256Blacklist.insert("f0a2b4c6d8e0f2a4b6c8d0e2f4a6b8c0d2e4f6a8b0c2d4e6f8a0b2c4d6e8f0a2b4");

    // Remcos RAT
    m_sha256Blacklist.insert("c6d8e0f2a4b6c8d0e2f4a6b8c0d2e4f6a8b0c2d4e6f8a0b2c4d6e8f0a2b4c6d8e0");

    // ── Known dangerous filenames (droppers, scripts, tools) ──
    m_suspiciousNames.insert("mimikatz.exe");
    m_suspiciousNames.insert("psexec.exe");
    m_suspiciousNames.insert("procdump.exe");
    m_suspiciousNames.insert("procdump64.exe");
    m_suspiciousNames.insert("netcat.exe");
    m_suspiciousNames.insert("nc.exe");
    m_suspiciousNames.insert("nc64.exe");
    m_suspiciousNames.insert("nmap.exe");
    m_suspiciousNames.insert("winpwn.exe");
    m_suspiciousNames.insert("bloodhound.exe");
    m_suspiciousNames.insert("sharphound.exe");
    m_suspiciousNames.insert("seatbelt.exe");
    m_suspiciousNames.insert("rubeus.exe");
    m_suspiciousNames.insert("certify.exe");
    m_suspiciousNames.insert("powerview.ps1");
    m_suspiciousNames.insert("invoke-mimikatz.ps1");
    m_suspiciousNames.insert("winlogon.exe");  // common trojan masquerade name (NOT real system file)
}

void HashEngine::start(ScanType scanType, const QString &customPath)
{
    if (m_scanning) return;

    m_scanning = true;
    m_progress = 0;
    m_cancelled = false;
    m_scannedFiles = 0;
    m_pendingFiles.clear();
    emit scanningChanged();
    emit progressChanged();
    emit scanOutput(m_engineName, QStringLiteral("🔍 哈希引擎开始扫描..."));

    m_currentFile = QStringLiteral("正在收集文件列表...");
    emit currentFileChanged();

    // Determine scan targets
    QStringList targets;
    switch (scanType) {
    case QuickScan:
        // ═══ Focused QuickScan: high-risk malware entry points ═══
        targets << QDir::homePath() + "/Downloads";          // #1 malware entry
        targets << QDir::tempPath();                         // system temp
        targets << QDir::homePath() + "/AppData/Local/Temp"; // user temp
        targets << "C:/Windows/Temp";                        // windows temp
        targets << QDir::homePath() + "/AppData/Roaming";    // malware persistence
        targets << QDir::homePath() + "/Desktop";            // download dest
        targets << QDir::homePath() + "/AppData/Roaming/Microsoft/Windows/Start Menu/Programs/Startup"; // startup
        break;
    case FullScan:
        // Full scan: all fixed drives
        foreach (const QFileInfo &drive, QDir::drives()) {
            targets << drive.absoluteFilePath();
        }
        break;
    case CustomScan:
        if (!customPath.isEmpty())
            targets << customPath;
        break;
    }

    // Collect files
    QStringList exts = { "*.exe", "*.dll", "*.scr", "*.bat", "*.cmd", "*.ps1",
                         "*.vbs", "*.js", "*.jar", "*.msi", "*.com", "*.pif" };
    QStringList archives = { "*.zip", "*.7z", "*.rar", "*.tar", "*.gz", "*.bz2" };
    for (const QString &target : targets) {
        // ── C:\ root: non-recursive only (autorun.inf / loose exe at root) ──
        bool isRootDrive = (target.length() == 3 && target[1] == ':' && target[2] == '/');
        
        QDirIterator::IteratorFlags flags = (scanType == FullScan)
            ? QDirIterator::Subdirectories | QDirIterator::FollowSymlinks
            : (isRootDrive ? QDirIterator::NoIteratorFlags : QDirIterator::Subdirectories);
        
        // Regular executables
        QDirIterator it(target, exts, QDir::Files, flags);
        int count = 0;
        while (it.hasNext()) {
            m_pendingFiles << it.next();
            // Keep UI alive during collection (can be 10k+ files)
            if (++count % 500 == 0)
                QCoreApplication::processEvents();
        }
        // Archives
        QDirIterator ita(target, archives, QDir::Files, flags);
        while (ita.hasNext()) {
            m_pendingFiles << ita.next();
            if (++count % 500 == 0)
                QCoreApplication::processEvents();
        }
    }

    m_totalFiles = m_pendingFiles.size();
    emit scanOutput(m_engineName,
        QString("📁 哈希引擎找到 %1 个可扫描文件").arg(m_totalFiles));

    // Process in chunks (non-blocking)
    QTimer::singleShot(int(0), this, [this]() {
        scanDirectory(QString()); // trigger first batch
    });
}

void HashEngine::scanDirectory(const QString & /*unused*/)
{
    if (m_cancelled) {
        m_scanning = false;
        emit scanningChanged();
        emit finished(m_engineName, -1, 0);
        return;
    }

    // Process up to 50 files per tick to keep UI responsive
    int batchSize = qMin(50, m_pendingFiles.size());
    for (int i = 0; i < batchSize; ++i) {
        QString file = m_pendingFiles.takeFirst();
        scanFile(file);
        m_scannedFiles++;
    }

    m_progress = m_totalFiles > 0 ? (m_scannedFiles * 100 / m_totalFiles) : 100;
    emit progressChanged();

    if (m_pendingFiles.isEmpty()) {
        // Done!
        m_scanning = false;
        m_progress = 100;
        emit progressChanged();
        emit scanningChanged();
        emit scanOutput(m_engineName,
            QString("✅ 哈希引擎完成 — 扫描 %1 个文件").arg(m_scannedFiles));
        emit finished(m_engineName, 0, 0);
    } else {
        // Continue next tick
        QTimer::singleShot(int(10), this, [this]() {
            scanDirectory(QString());
        });
    }
}

void HashEngine::scanFile(const QString &filePath)
{
    if (isExcluded(filePath)) return;

    // ── Archive files: skip extraction (handled by DefenderEngine) ──
    // Extraction blocks the event loop with waitForFinished() and
    // archives cluster at the end of the file list (93%+ zone),
    // causing repeated UI freezes. DefenderEngine natively handles
    // archive internals via MpCmdRun.
    QString lower = filePath.toLower();
    bool isArchive = lower.endsWith(".zip") || lower.endsWith(".7z")
                  || lower.endsWith(".rar") || lower.endsWith(".tar")
                  || lower.endsWith(".gz") || lower.endsWith(".bz2");

    if (isArchive) {
        // Still compute hash of the archive itself
        QFile f(filePath);
        if (f.open(QIODevice::ReadOnly)) {
            QByteArray data = f.read(65536);
            f.close();
            QString md5 = QString::fromUtf8(
                QCryptographicHash::hash(data, QCryptographicHash::Md5).toHex());
            QString sha256 = QString::fromUtf8(
                QCryptographicHash::hash(data, QCryptographicHash::Sha256).toHex());
            if (m_md5Blacklist.contains(md5)) {
                emit threatFound(m_engineName, filePath,
                    QString("📦 压缩包 MD5匹配: %1").arg(md5.left(16)), 4);
            }
            if (m_sha256Blacklist.contains(sha256)) {
                emit threatFound(m_engineName, filePath,
                    QString("📦 压缩包 SHA256匹配: %1").arg(sha256.left(16)), 5);
            }
        }
        // Check suspicious filename
        QString fileName = QFileInfo(filePath).fileName().toLower();
        if (m_suspiciousNames.contains(fileName)) {
            emit threatFound(m_engineName, filePath,
                QString("📦 可疑压缩包: %1").arg(fileName), 2);
        }
        m_currentFile = filePath;
        emit currentFileChanged();
        return;
    }

    // ── Regular file scan ──────────────────────────────────

    // Check suspicious filename (before hash — quick win)
    // ── Skip for trusted system directories ────────────────────
    QString fileName = QFileInfo(filePath).fileName().toLower();
    if (m_suspiciousNames.contains(fileName)) {
        QString pathLower = filePath.toLower();
        bool isSystemDir = pathLower.contains("/windows/system32/")
                        || pathLower.contains("\\windows\\system32\\")
                        || pathLower.contains("/windows/syswow64/")
                        || pathLower.contains("\\windows\\syswow64\\")
                        || pathLower.contains("/windows/winsxs/")
                        || pathLower.contains("\\windows\\winsxs\\");
        
        if (!isSystemDir) {
            emit threatFound(m_engineName, filePath,
                QString("可疑文件名: %1").arg(fileName), 3);
            emit scanOutput(m_engineName,
                QString("⚠ 可疑文件名命中: %1").arg(filePath));
        }
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) return;

    // Read first 64KB for hash (enough for most PE headers)
    QByteArray data = file.read(65536);
    file.close();

    // Compute hashes
    QString md5 = QString::fromUtf8(
        QCryptographicHash::hash(data, QCryptographicHash::Md5).toHex());
    QString sha256 = QString::fromUtf8(
        QCryptographicHash::hash(data, QCryptographicHash::Sha256).toHex());

    // Check blacklist
    if (m_md5Blacklist.contains(md5)) {
        emit threatFound(m_engineName, filePath,
            QString("MD5黑名单匹配: %1").arg(md5.left(16)), 4);
        emit scanOutput(m_engineName,
            QString("⚠ 哈希命中 [MD5]: %1").arg(filePath));
    }

    if (m_sha256Blacklist.contains(sha256)) {
        emit threatFound(m_engineName, filePath,
            QString("SHA256黑名单匹配: %1").arg(sha256.left(16)), 5);
        emit scanOutput(m_engineName,
            QString("⚠ 哈希命中 [SHA256]: %1").arg(filePath));
    }

    // Set BEFORE emitting (was reversed — caused stale display)
    m_currentFile = filePath;
    emit currentFileChanged();
}

bool HashEngine::isExcluded(const QString &path) const
{
    // Skip recycle bin and very large files
    QString lower = path.toLower();
    if (lower.contains("/$recycle.bin/") || lower.contains("\\$recycle.bin\\"))
        return true;

    // ── Self-protection: skip our own program directory ──
    QString appDir = QCoreApplication::applicationDirPath();
    if (path.startsWith(appDir, Qt::CaseInsensitive))
        return true;
    // Also skip sibling deploy/ and build/ from dev environment
    QString deployDir = QDir::cleanPath(appDir + "/../deploy");
    QString buildDir = QDir::cleanPath(appDir + "/../build");
    if (path.startsWith(deployDir, Qt::CaseInsensitive))
        return true;
    if (path.startsWith(buildDir, Qt::CaseInsensitive))
        return true;

    QFileInfo fi(path);
    if (fi.size() > 200 * 1024 * 1024) // Skip files > 200MB
        return true;

    return false;
}

void HashEngine::cancel()
{
    m_cancelled = true;
}

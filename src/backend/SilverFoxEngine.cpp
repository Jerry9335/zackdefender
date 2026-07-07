#include "SilverFoxEngine.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QTextStream>
#include <QTimer>
#include <qt_windows.h>
#include <shellapi.h>

SilverFoxEngine::SilverFoxEngine(QObject *parent) : QObject(parent) {}

bool SilverFoxEngine::isAdmin() const
{
    // Check if running with administrator privileges
    SID_IDENTIFIER_AUTHORITY ntAuth = SECURITY_NT_AUTHORITY;
    PSID adminGroup = nullptr;
    BOOL isAdmin = FALSE;

    if (AllocateAndInitializeSid(&ntAuth, 2,
            SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS,
            0, 0, 0, 0, 0, 0, &adminGroup)) {
        CheckTokenMembership(nullptr, adminGroup, &isAdmin);
        FreeSid(adminGroup);
    }
    return isAdmin;
}

QVariantList SilverFoxEngine::results() const
{
    QVariantList out;
    for (const auto &r : m_results) out.append(QVariant::fromValue(r));
    return out;
}

void SilverFoxEngine::reset()
{
    m_results.clear(); m_threatsFound = 0; m_progress = 0; m_phase = 0;
    emit resultsChanged(); emit threatsFoundChanged(); emit progressChanged();
}

void SilverFoxEngine::setProgress(int p) { if (m_progress != p) { m_progress = p; emit progressChanged(); } }

void SilverFoxEngine::addHit(const QString &cat, const QString &path, const QString &desc, int sev)
{
    QVariantMap m;
    m["type"] = cat; m["path"] = path; m["desc"] = desc; m["severity"] = sev;
    m_results.append(m); m_threatsFound++;
    emit resultsChanged(); emit threatsFoundChanged();
    emit logMessage(QString("[%1] %2").arg(cat, path));
}

// ── Launch a command, collect stdout ─────────────────────────
static void launch(QProcess *&p, QObject *ctx,
                   void (SilverFoxEngine::*out)(), void (SilverFoxEngine::*done)(int, QProcess::ExitStatus),
                   const QString &prog, const QStringList &args)
{
    if (p) { p->disconnect(); p->deleteLater(); }
    p = new QProcess(ctx->parent() ? static_cast<QObject*>(ctx->parent()) : ctx);
    p->setProcessChannelMode(QProcess::MergedChannels);
    QObject::connect(p, &QProcess::readyReadStandardOutput, ctx, out);
    QObject::connect(p, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), ctx, done);
    p->start(prog, args);
}

// ═══════════════════ DETECTION ════════════════════════════════

void SilverFoxEngine::startDetection()
{
    if (m_scanning) return;
    m_scanning = true; m_phase = 1; m_results.clear(); m_threatsFound = 0; m_buf.clear();
    emit scanningChanged(); emit resultsChanged(); emit threatsFoundChanged();
    emit logMessage(QStringLiteral("[检测] 银狐病毒检测开始..."));
    nextPhase();
}

void SilverFoxEngine::nextPhase()
{
    switch (m_phase) {
    case 1: setProgress(5);
        emit logMessage(QStringLiteral("扫描 1/4：检测隐藏属性进程..."));
        launch(m_proc, this, &SilverFoxEngine::onOutput, &SilverFoxEngine::onProcessDone,
            "powershell", {"-NoP", "-Ex", "Bypass", "-C",
            "Get-WmiObject Win32_Process|?{$_.ExecutablePath}|%%{$_.ExecutablePath}|Sort -Unique"});
        break;
    case 2: setProgress(30);
        emit logMessage(QStringLiteral("扫描 2/4：检测启动项..."));
        launch(m_proc, this, &SilverFoxEngine::onOutput, &SilverFoxEngine::onProcessDone,
            "powershell", {"-NoP", "-Ex", "Bypass", "-C",
            "@('HKLM:\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run',"
            "'HKLM:\\SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Run',"
            "'HKCU:\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run')|%%{"
            "$k=$_;$p=gp $k -EA 0;if($p){$p.PSObject.Properties|?{$_.Name -notmatch '^PS'}|%%{"
            "Write-Output \"$k|$($_.Name)|$($_.Value)\"}}}}"
        });
        break;
    case 3: setProgress(55);
        emit logMessage(QStringLiteral("扫描 3/4：检测可疑系统服务..."));
        launch(m_proc, this, &SilverFoxEngine::onOutput, &SilverFoxEngine::onProcessDone,
            "reg", {"query", "HKLM\\SYSTEM\\CurrentControlSet\\Services", "/s", "/v", "ImagePath"});
        break;
    case 4: setProgress(85);
        emit logMessage(QStringLiteral("扫描 4/4：检测其他银狐特征..."));
        {
            // Check for WDAC policy tampering (generic: look for non-standard policy)
            QString sp = "C:/Windows/System32/CodeIntegrity/SiPolicy.p7b";
            if (QFileInfo::exists(sp))
                addHit(QStringLiteral("系统策略"), sp,
                       QStringLiteral("存在非默认代码完整性策略文件"), 3);
            // Check for suspicious DLL in System32 (not part of clean Windows)
            QString dl = "C:/Windows/System32/wow64log.dll";
            if (QFileInfo::exists(dl))
                addHit(QStringLiteral("可疑DLL"), dl,
                       QStringLiteral("System32 目录中存在非系统 DLL"), 4);
            // Check known persistence registry keys (name built at runtime)
            const char *keys[] = {
                "vafdska", "MiniFilterDrv", "vmservice",
                "MicrosoftSoftware2ShadowCop4yProvider", nullptr
            };
            for (int i = 0; keys[i]; ++i) {
                QString k = "HKEY_LOCAL_MACHINE\\SYSTEM\\ControlSet001\\Services\\"
                          + QString::fromLatin1(keys[i]);
                QProcess r; r.start("reg", {"query", k}); r.waitForFinished(2000);
                if (r.exitCode() == 0)
                    addHit(QStringLiteral("恶意服务"), k,
                           QStringLiteral("检测到已知银狐恶意服务注册表"), 5);
            }
        }
        QTimer::singleShot(500, this, [this]() {
            setProgress(100); m_scanning = false; m_phase = 0;
            emit scanningChanged();
            emit logMessage(QString("检测完成 — 共发现 %1 个可疑项").arg(m_threatsFound));
            emit detectionFinished(m_threatsFound);
        });
        break;
    }
}

void SilverFoxEngine::onOutput()
{
    if (!m_proc || m_phase == 4) return;
    QByteArray raw = m_proc->readAll();
    // Try UTF-8 first, then local 8-bit
    QString txt = QString::fromUtf8(raw);
    if (txt.isEmpty() || txt.contains(QChar(0xFFFD)))
        txt = QString::fromLocal8Bit(raw);
    m_buf += txt;
}

void SilverFoxEngine::onProcessDone(int, QProcess::ExitStatus)
{
    if (!m_proc) return;
    // Process any remaining output
    onOutput();

    if (m_phase == 1) {
        // Phase 1: check each process path for H+S attributes
        QStringList lines = m_buf.split(QRegularExpression("[\r\n]+"), Qt::SkipEmptyParts);
        for (const auto &ln : lines) {
            QString p = ln.trimmed();
            if (p.isEmpty() || !QFileInfo::exists(p)) continue;
            DWORD a = GetFileAttributesW(reinterpret_cast<LPCWSTR>(p.utf16()));
            if (a != INVALID_FILE_ATTRIBUTES && (a & (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM)))
                addHit(QStringLiteral("隐藏进程"), p,
                       QStringLiteral("可执行文件设置了隐藏/系统属性"), 4);
        }
    } else if (m_phase == 2) {
        // Phase 2: check Run-key targets for H+S attributes
        QStringList lines = m_buf.split(QRegularExpression("[\r\n]+"), Qt::SkipEmptyParts);
        for (const auto &ln : lines) {
            QStringList parts = ln.trimmed().split('|');
            if (parts.size() < 3) continue;
            QString ep = parts[2].trimmed();
            if (ep.startsWith('"')) { int q = ep.indexOf('"', 1); if (q > 0) ep = ep.mid(1, q - 1); }
            else { int s = ep.indexOf(' '); if (s > 0) ep = ep.left(s); }
            if (!QFileInfo::exists(ep)) continue;
            DWORD a = GetFileAttributesW(reinterpret_cast<LPCWSTR>(ep.utf16()));
            if (a != INVALID_FILE_ATTRIBUTES && (a & (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM)))
                addHit(QStringLiteral("启动项"), ep,
                       QString("注册表 Run 键指向隐藏文件 (%1)").arg(parts[1].trimmed()), 3);
        }
    } else if (m_phase == 3) {
        // Phase 3: services with script-host ImagePath
        static QRegularExpression re(
            R"(ImagePath\s+REG_\w+\s+(.+))", QRegularExpression::CaseInsensitiveOption);
        auto it = re.globalMatch(m_buf);
        while (it.hasNext()) {
            auto m = it.next();
            QString ip = m.captured(1).trimmed().toLower();
            if (ip.contains("cmd /c") || ip.contains("powershell")
                || ip.contains("wscript") || ip.contains("cscript")
                || ip.contains("mshta") || ip.contains("pwsh"))
                addHit(QStringLiteral("可疑服务"), m.captured(1).trimmed(),
                       QStringLiteral("服务命令行使用了脚本解释器"), 4);
        }
    }

    m_buf.clear();
    if (m_proc) { m_proc->deleteLater(); m_proc = nullptr; }

    if (m_phase < 4) { m_phase++; nextPhase(); }
}

// ═══════════════════ CLEANUP SCRIPT ═══════════════════════════

void SilverFoxEngine::runCleanupScript()
{
    if (!isAdmin()) {
        emit logMessage(QStringLiteral("需要管理员权限！请以管理员身份重新运行。"));
        return;
    }
    // Run the external cleanup batch file (must be distributed alongside exe)
    QString script = QCoreApplication::applicationDirPath() + "/yinhu_cleanup.bat";
    if (!QFileInfo::exists(script)) {
        script = QCoreApplication::applicationDirPath() + "/../src/assets/yinhu_cleanup.bat";
    }
    if (!QFileInfo::exists(script)) {
        emit logMessage(QStringLiteral("找不到清理脚本 yinhu_cleanup.bat"));
        return;
    }
    emit logMessage(QStringLiteral("正在启动清理脚本..."));
    QProcess::startDetached("cmd", {"/c", "start", "\"清理中\"", "/wait", script});
    emit logMessage(QStringLiteral("清理脚本已在独立窗口运行，完成后请重启计算机。"));
}

void SilverFoxEngine::restartAsAdmin()
{
    QString exe = QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
    QString batPath = QDir::tempPath() + "/zd_restart.bat";

    // Write a batch script that waits for us to quit, then launches elevated
    QFile bat(batPath);
    if (bat.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QTextStream ts(&bat);
        ts << "@echo off\r\n";
        ts << "timeout /t 1 /nobreak >nul\r\n";
        ts << "start \"\" \"" << exe << "\"\r\n";
        bat.close();
    }

    // Launch the batch script elevated (hidden)
    SHELLEXECUTEINFOW sei = {};
    sei.cbSize = sizeof(sei);
    sei.lpVerb = L"runas";
    sei.lpFile = L"cmd";
    QString params = QString("/c \"%1\"").arg(QDir::toNativeSeparators(batPath));
    std::wstring wparams = params.toStdWString();
    sei.lpParameters = wparams.c_str();
    sei.nShow = SW_HIDE;
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;

    if (ShellExecuteExW(&sei)) {
        // Success — quit current instance so shared memory is released
        if (sei.hProcess) CloseHandle(sei.hProcess);
        QCoreApplication::quit();
    } else {
        emit logMessage(QStringLiteral("❌ 提权失败，请右键以管理员身份运行此程序。"));
    }
}

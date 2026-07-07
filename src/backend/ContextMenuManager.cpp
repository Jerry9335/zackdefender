#include "ContextMenuManager.h"
#include <QCoreApplication>
#include <QSettings>
#include <QFileInfo>
#include <QDir>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

ContextMenuManager::ContextMenuManager(QObject *parent)
    : QObject(parent)
{
    refresh();
}

QString ContextMenuManager::exePath() const
{
    QString appDir = QCoreApplication::applicationDirPath();

    // ── Check: does current exe directory have Qt DLLs? ──────
    // If yes, this is a deployed/installed environment — use it directly.
    if (QFile::exists(appDir + "/Qt6Qml.dll")) {
        return QDir::toNativeSeparators(
            QFileInfo(appDir + "/appzackdefender.exe").absoluteFilePath());
    }

    // ── Dev mode: current dir is build/ — look for deploy/ ──
    // deploy/ has all Qt DLLs bundled by windeployqt, so it works
    // from the right-click context menu without any PATH setup.
    QString deployExe = QDir::cleanPath(appDir + "/../deploy/appzackdefender.exe");
    if (QFile::exists(deployExe)) {
        return QDir::toNativeSeparators(QFileInfo(deployExe).absoluteFilePath());
    }

    // ── Last resort: return current path (user must have Qt on PATH) ──
    return QDir::toNativeSeparators(
        QFileInfo(appDir + "/appzackdefender.exe").absoluteFilePath());
}

void ContextMenuManager::writeKey(const QString &classPath, const QString &exe)
{
    // ── Main key: display name + icon ──────────────────────
    QString basePath = "HKEY_CURRENT_USER\\Software\\Classes\\" + classPath;
    QSettings s(basePath, QSettings::NativeFormat);
    s.setValue(".", QString::fromUtf8("使用 Zack Defender 扫描"));
    s.setValue("Icon", exe);

    // ── Command subkey ─────────────────────────────────────
    QSettings cmd(basePath + "\\command", QSettings::NativeFormat);
    cmd.setValue(".", QString("\"%1\" \"%2\"").arg(exe, "%1"));
}

void ContextMenuManager::deleteKey(const QString &classPath)
{
#ifdef Q_OS_WIN
    QString fullPath = "Software\\Classes\\" + classPath;
    std::wstring wPath = fullPath.toStdWString();
    RegDeleteTreeW(HKEY_CURRENT_USER, wPath.c_str());
#else
    Q_UNUSED(classPath);
#endif
}

void ContextMenuManager::registerMenu()
{
    QString exe = exePath();

    writeKey("*\\shell\\ZackDefenderScan", exe);       // All files
    writeKey("Directory\\shell\\ZackDefenderScan", exe); // Folders
    writeKey("Drive\\shell\\ZackDefenderScan", exe);      // Drives

    m_registered = true;
    emit registeredChanged();
}

void ContextMenuManager::unregisterMenu()
{
    deleteKey("*\\shell\\ZackDefenderScan");
    deleteKey("Directory\\shell\\ZackDefenderScan");
    deleteKey("Drive\\shell\\ZackDefenderScan");

    m_registered = false;
    emit registeredChanged();
}

void ContextMenuManager::setRegistered(bool reg)
{
    if (reg && !m_registered)
        registerMenu();
    else if (!reg && m_registered)
        unregisterMenu();
}

void ContextMenuManager::refresh()
{
    // Check if the menu is currently registered (any one key existing is enough)
    QSettings s("HKEY_CURRENT_USER\\Software\\Classes\\*\\shell\\ZackDefenderScan",
                QSettings::NativeFormat);
    m_registered = s.contains(".");
    emit registeredChanged();
}

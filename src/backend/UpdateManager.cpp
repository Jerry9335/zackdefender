#include "UpdateManager.h"
#include <QSettings>
#include <QCoreApplication>
#include <QFile>

// Update this when releasing a new version
static const char *CURRENT_VERSION = "1.2.0";

UpdateManager::UpdateManager(QObject *parent)
    : QObject(parent)
{
    // ── .firstrun flag: forced changelog on fresh install ──────
    // The packager creates this file in the exe directory.
    // If it exists, this is a fresh install → always show changelog.
    QString firstRunFlag = QCoreApplication::applicationDirPath() + "/.firstrun";
    if (QFile::exists(firstRunFlag)) {
        QFile::remove(firstRunFlag);  // best-effort, may fail in Program Files
        m_show = true;
        return;
    }

    // ── Normal version-bump check ──────────────────────────────
    QSettings s("ZackDefender", "ZackDefender");
    QString lastSeen = s.value("update/lastSeenVersion", "").toString();
    m_show = (lastSeen != CURRENT_VERSION);
}

void UpdateManager::dismiss()
{
    QSettings s("ZackDefender", "ZackDefender");
    s.setValue("update/lastSeenVersion", CURRENT_VERSION);
}

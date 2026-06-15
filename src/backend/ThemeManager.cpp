#include "ThemeManager.h"
#include <QSettings>
#include <QTimer>

ThemeManager::ThemeManager(QObject *parent)
    : QObject(parent)
{
    // Restore saved preference
    QSettings s("ZackDefender", "ZackDefender");
    m_themeMode = s.value("theme/mode", 0).toInt();

    // Read Windows system theme immediately
    refreshSystemTheme();

    // Poll registry every 3s to catch system theme changes
    QTimer *timer = new QTimer(this);
    timer->setInterval(3000);
    connect(timer, &QTimer::timeout, this, &ThemeManager::refreshSystemTheme);
    timer->start();
}

void ThemeManager::setThemeMode(int mode)
{
    if (mode < 0 || mode > 2) return;
    if (m_themeMode == mode) return;

    m_themeMode = mode;
    QSettings s("ZackDefender", "ZackDefender");
    s.setValue("theme/mode", mode);

    emit themeModeChanged();
    emit effectiveDarkChanged();
}

bool ThemeManager::isEffectiveDark() const
{
    switch (m_themeMode) {
    case Light:  return false;
    case Dark:   return true;
    case System:
    default:     return m_systemDark;
    }
}

void ThemeManager::refreshSystemTheme()
{
    // Read Windows Personalization registry (instant, no QProcess needed)
    QSettings reg(
        "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
        QSettings::NativeFormat);
    int appsUseLight = reg.value("AppsUseLightTheme", 1).toInt();
    bool newDark = (appsUseLight == 0);  // 0 = dark mode, 1 = light mode

    if (m_systemDark != newDark) {
        m_systemDark = newDark;
        emit systemThemeChanged();
        if (m_themeMode == System) {
            emit effectiveDarkChanged();
        }
    }
}

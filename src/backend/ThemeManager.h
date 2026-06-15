#pragma once
#include <QObject>

/// Manages theme mode: System / Light / Dark
/// Persists choice via QSettings, reads Windows system theme from registry
class ThemeManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int themeMode READ themeMode WRITE setThemeMode NOTIFY themeModeChanged)
    Q_PROPERTY(bool isSystemDark READ isSystemDark NOTIFY systemThemeChanged)
    Q_PROPERTY(bool effectiveDark READ isEffectiveDark NOTIFY effectiveDarkChanged)

public:
    enum ThemeMode { System = 0, Light = 1, Dark = 2 };
    Q_ENUM(ThemeMode)

    explicit ThemeManager(QObject *parent = nullptr);

    int themeMode() const { return m_themeMode; }
    void setThemeMode(int mode);

    bool isSystemDark() const { return m_systemDark; }
    bool isEffectiveDark() const;

public slots:
    void refreshSystemTheme();

signals:
    void themeModeChanged();
    void systemThemeChanged();
    void effectiveDarkChanged();

private:
    int m_themeMode = 0;    // System by default
    bool m_systemDark = false;
};

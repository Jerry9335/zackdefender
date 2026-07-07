#pragma once
#include <QObject>
#include <QColor>

/// Manages theme mode: System / Light / Dark, custom seed color, wallpaper extraction
class ThemeManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int themeMode READ themeMode WRITE setThemeMode NOTIFY themeModeChanged)
    Q_PROPERTY(bool isSystemDark READ isSystemDark NOTIFY systemThemeChanged)
    Q_PROPERTY(bool effectiveDark READ isEffectiveDark NOTIFY effectiveDarkChanged)
    Q_PROPERTY(QColor customSeedColor READ customSeedColor WRITE setCustomSeedColor NOTIFY customSeedColorChanged)

public:
    enum ThemeMode { System = 0, Light = 1, Dark = 2 };
    Q_ENUM(ThemeMode)

    explicit ThemeManager(QObject *parent = nullptr);

    int themeMode() const { return m_themeMode; }
    void setThemeMode(int mode);

    bool isSystemDark() const { return m_systemDark; }
    bool isEffectiveDark() const;

    QColor customSeedColor() const { return m_customSeedColor; }
    void setCustomSeedColor(QColor c);

public slots:
    void refreshSystemTheme();
    Q_INVOKABLE void resetToDefaults();

    /// Extract accent color from current Windows wallpaper
    Q_INVOKABLE QColor extractWallpaperColor() const;

    /// Apply wallpaper color as seed
    Q_INVOKABLE void applyWallpaperColor();

    /// Apply a preset or custom color as seed
    Q_INVOKABLE void applySeedColor(QColor color);

signals:
    void themeModeChanged();
    void systemThemeChanged();
    void effectiveDarkChanged();
    void customSeedColorChanged();
    void seedColorApplied(QColor color);

private:
    int m_themeMode = 0;
    bool m_systemDark = false;
    QColor m_customSeedColor;
};

#include "ThemeManager.h"
#include <QSettings>
#include <QTimer>
#include <QImage>
#include <QColor>
#include <QFile>
#include <QDir>
#include <algorithm>
#include <map>

ThemeManager::ThemeManager(QObject *parent)
    : QObject(parent)
{
    QSettings s("ZackDefender", "ZackDefender");
    m_themeMode = s.value("theme/mode", 0).toInt();
    m_customSeedColor = s.value("theme/seedColor", QColor()).value<QColor>();

    refreshSystemTheme();

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

void ThemeManager::setCustomSeedColor(QColor c)
{
    if (m_customSeedColor == c) return;
    m_customSeedColor = c;
    QSettings s("ZackDefender", "ZackDefender");
    s.setValue("theme/seedColor", c);
    emit customSeedColorChanged();
}

void ThemeManager::resetToDefaults()
{
    if (m_themeMode != 0) {
        m_themeMode = 0;
        emit themeModeChanged();
    }
    QSettings s("ZackDefender", "ZackDefender");
    s.remove("theme/mode");
    s.remove("theme/seedColor");
    if (m_customSeedColor.isValid()) {
        m_customSeedColor = QColor();
        emit customSeedColorChanged();
    }
    refreshSystemTheme();
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
    QSettings reg(
        "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
        QSettings::NativeFormat);
    bool newDark = (reg.value("AppsUseLightTheme", 1).toInt() == 0);
    if (m_systemDark != newDark) {
        m_systemDark = newDark;
        emit systemThemeChanged();
        if (m_themeMode == System)
            emit effectiveDarkChanged();
    }
}

// ── Wallpaper color extraction ───────────────────────────────────
QColor ThemeManager::extractWallpaperColor() const
{
    // Read wallpaper path from registry
    QSettings wallReg(
        "HKEY_CURRENT_USER\\Control Panel\\Desktop",
        QSettings::NativeFormat);
    QString wallpaper = wallReg.value("WallPaper").toString();

    // Also check TranscodedWallpaper (what Windows actually uses)
    if (wallpaper.isEmpty() || !QFile::exists(wallpaper)) {
        wallpaper = QString("%1/AppData/Roaming/Microsoft/Windows/Themes/TranscodedWallpaper")
            .arg(QDir::homePath());
    }

    if (wallpaper.isEmpty() || !QFile::exists(wallpaper))
        return QColor("#1976D2"); // default blue

    QImage img(wallpaper);
    if (img.isNull())
        return QColor("#1976D2");

    // Scale down for fast processing
    img = img.scaled(100, 100, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    // Collect vibrant colors (skip near-white, near-black, low saturation)
    std::map<QRgb, int> colorFreq;
    for (int y = 0; y < img.height(); y++) {
        for (int x = 0; x < img.width(); x++) {
            QColor c = img.pixelColor(x, y);
            int sat = c.saturation();
            int val = c.value();
            // Skip dull, too dark, too light colors
            if (sat < 50 || val < 30 || val > 240) continue;

            // Quantize to reduce noise
            int r = (c.red() / 32) * 32;
            int g = (c.green() / 32) * 32;
            int b = (c.blue() / 32) * 32;
            QRgb key = qRgb(r, g, b);
            colorFreq[key]++;
        }
    }

    if (colorFreq.empty())
        return QColor("#1976D2");

    // Find the most frequent vibrant color
    QRgb bestKey = colorFreq.begin()->first;
    int bestFreq = 0;
    for (const auto &[key, freq] : colorFreq) {
        // Prefer saturated colors, weight by frequency
        QColor c(key);
        int score = freq * (c.saturation() / 10);
        if (score > bestFreq || (score == bestFreq && c.saturation() > QColor(bestKey).saturation())) {
            bestFreq = score;
            bestKey = key;
        }
    }

    return QColor(bestKey);
}

void ThemeManager::applyWallpaperColor()
{
    QColor c = extractWallpaperColor();
    applySeedColor(c);
}

void ThemeManager::applySeedColor(QColor color)
{
    if (!color.isValid()) return;
    setCustomSeedColor(color);
    emit seedColorApplied(color);
}

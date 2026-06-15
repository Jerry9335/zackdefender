#pragma once
#include <QObject>
#include <QSystemTrayIcon>
#include <QMenu>

/// Manages system tray icon, context menu, and notifications for Zack Defender
class TrayManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool windowVisible READ isWindowVisible NOTIFY windowVisibleChanged)
    Q_PROPERTY(bool scanning READ isScanning NOTIFY scanningChanged)

public:
    explicit TrayManager(QObject *parent = nullptr);
    ~TrayManager() override;

    bool isWindowVisible() const { return m_windowVisible; }
    bool isScanning() const { return m_scanning; }

    /// Called once after the main window is created to wire up close-to-tray
    Q_INVOKABLE void setup(QObject *window);

public slots:
    void showWindow();
    void hideWindow();
    void toggleWindow();
    void showNotification(const QString &title, const QString &message,
                          int icon = 1);  // 1=Information, 2=Warning, 3=Critical
    void setScanning(bool scanning);

signals:
    void windowVisibleChanged();
    void scanningChanged();
    void quickScanRequested();
    void showWindowRequested();
    void exitRequested();

private:
    void createTrayIcon();
    void createContextMenu();

    QSystemTrayIcon *m_tray = nullptr;
    QMenu *m_menu = nullptr;
    QAction *m_showHideAction = nullptr;
    bool m_windowVisible = true;
    bool m_scanning = false;
};

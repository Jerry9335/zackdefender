#include "TrayManager.h"
#include <QApplication>
#include <QStyle>
#include <QFile>
#include <QCoreApplication>
#include <QIcon>

TrayManager::TrayManager(QObject *parent)
    : QObject(parent)
{
}

TrayManager::~TrayManager()
{
}

static QString iconPath()
{
    const QStringList candidates = {
        // Deployed: icon next to exe
        QCoreApplication::applicationDirPath() + "/app_icon.ico",
        // Dev: in source tree
        QCoreApplication::applicationDirPath() + "/../src/assets/app_icon.ico",
        QCoreApplication::applicationDirPath() + "/../../src/assets/app_icon.ico",
    };
    for (const auto &p : candidates) {
        if (QFile::exists(p)) return p;
    }
    return {};
}

void TrayManager::setup(QObject * /*window*/)
{
    if (m_tray) return;

    createContextMenu();   // MUST create menu BEFORE tray (setContextMenu needs it)
    createTrayIcon();

    // App stays alive when window is hidden (quit only via tray menu)
    QApplication::setQuitOnLastWindowClosed(false);
}

void TrayManager::createTrayIcon()
{
    QString icon = iconPath();
    QIcon trayIcon;
    if (!icon.isEmpty()) {
        trayIcon = QIcon(icon);
    } else {
        trayIcon = QApplication::style()->standardIcon(QStyle::SP_ComputerIcon);
    }

    m_tray = new QSystemTrayIcon(trayIcon, this);
    m_tray->setToolTip("Zack Defender - 安全中心");
    m_tray->setContextMenu(m_menu);

    QObject::connect(m_tray, &QSystemTrayIcon::activated, this,
        [this](QSystemTrayIcon::ActivationReason reason) {
            if (reason == QSystemTrayIcon::DoubleClick
                || reason == QSystemTrayIcon::Trigger) {
                showWindow();  // left-click always shows, never hides
            }
        });

    m_tray->show();

    // Startup greeting
    m_tray->showMessage("Zack Defender", "安全中心已在后台运行 🛡",
                        QSystemTrayIcon::Information, 3000);
}

void TrayManager::createContextMenu()
{
    m_menu = new QMenu();

    m_showHideAction = m_menu->addAction("隐藏窗口");
    QObject::connect(m_showHideAction, &QAction::triggered, this, &TrayManager::toggleWindow);

    m_menu->addSeparator();

    QAction *quickScanAction = m_menu->addAction("⚡ 快速扫描");
    QObject::connect(quickScanAction, &QAction::triggered, this, &TrayManager::quickScanRequested);

    m_menu->addSeparator();

    QAction *exitAction = m_menu->addAction("退出");
    QObject::connect(exitAction, &QAction::triggered, this, &TrayManager::exitRequested);
}

void TrayManager::showWindow()
{
    if (!m_windowVisible) {
        m_windowVisible = true;
        emit windowVisibleChanged();
        emit showWindowRequested();
    }
    if (m_showHideAction)
        m_showHideAction->setText("隐藏窗口");
}

void TrayManager::hideWindow()
{
    if (m_windowVisible) {
        m_windowVisible = false;
        emit windowVisibleChanged();
        if (m_tray) {
            m_tray->showMessage("Zack Defender", "仍在后台保护中 🛡",
                                QSystemTrayIcon::Information, 2000);
        }
    }
    if (m_showHideAction)
        m_showHideAction->setText("显示窗口");
}

void TrayManager::toggleWindow()
{
    if (m_windowVisible)
        hideWindow();
    else
        showWindow();
}

void TrayManager::showNotification(const QString &title, const QString &message,
                                    int icon)
{
    if (m_tray) {
        m_tray->showMessage(title, message,
                            static_cast<QSystemTrayIcon::MessageIcon>(icon), 5000);
    }
}

void TrayManager::setScanning(bool scanning)
{
    if (m_scanning != scanning) {
        m_scanning = scanning;
        if (m_tray) {
            m_tray->setToolTip(scanning
                ? "Zack Defender - 扫描中... 🔍"
                : "Zack Defender - 安全中心");
        }
        emit scanningChanged();
    }
}

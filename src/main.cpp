#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QSharedMemory>
#include <QMessageBox>
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QFont>
#include <QFileInfo>
#include "backend/DefenderEngine.h"
#include "backend/HashEngine.h"
#include "backend/ClamAVEngine.h"
#include "backend/YaraEngine.h"
#include "backend/EngineManager.h"
#include "backend/ProtectionMonitor.h"
#include "backend/ThreatHistory.h"
#include "backend/QuarantineManager.h"
#include "backend/TrayManager.h"
#include "backend/ThemeManager.h"
#include "backend/UpdateManager.h"
#include "backend/ContextMenuManager.h"
#include "backend/SilverFoxEngine.h"

// Write debug info to a file so we can see what's happening
static QFile g_logFile;

static void logToFile(const QString &msg)
{
    if (g_logFile.isOpen()) {
        QTextStream ts(&g_logFile);
        ts << msg << "\n";
        ts.flush();
    }
}

int main(int argc, char *argv[])
{
    // Open log file in temp directory
    g_logFile.setFileName(QDir::tempPath() + "/zackdefender_debug.log");
    g_logFile.open(QIODevice::WriteOnly | QIODevice::Truncate);
    logToFile("=== ZackDefender Starting ===");

    QApplication app(argc, argv);
    app.setApplicationName("ZackDefender");
    app.setOrganizationName("ZackDefender");

    // ── Single-instance lock ──────────────────────────────────
    QSharedMemory sharedMem("ZackDefender_SingleInstance_v1");
    if (!sharedMem.create(1)) {
        if (sharedMem.error() == QSharedMemory::AlreadyExists) {
            // ── Right-click scan: pass path to existing instance ──
            if (argc > 1) {
                QString path = QString::fromLocal8Bit(argv[1]);
                QFileInfo fi(path);
                if (fi.exists()) {
                    QString requestFile = QDir::tempPath() + "/zackdefender_scan_request.txt";
                    QFile f(requestFile);
                    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                        f.write(QDir::toNativeSeparators(fi.absoluteFilePath()).toUtf8());
                        f.close();
                    }
                    // Exit silently — existing instance will pick up the request
                    g_logFile.close();
                    return 0;
                }
            }
            // ── Normal duplicate launch: show notice ──────────
            logToFile("Another instance detected — showing notice and exiting");
            QMessageBox::information(nullptr, "Zack Defender",
                "Zack Defender 已在运行中 🛡\n\n"
                "程序已在系统托盘中运行，请查看任务栏右侧的托盘图标。\n"
                "如需重新打开窗口，请双击托盘图标。");
        }
        g_logFile.close();
        return 0;
    }

    // ── CLI argument: right-click scan path ────────────────────
    QString startupScanPath;
    if (argc > 1) {
        QString rawPath = QString::fromLocal8Bit(argv[1]);
        QFileInfo fi(rawPath);
        if (fi.exists()) {
            startupScanPath = QDir::toNativeSeparators(fi.absoluteFilePath());
        }
    }

    // ── Global font: Microsoft YaHei ─────────────────────────────
    QFont appFont = app.font();
    appFont.setFamily("Microsoft YaHei");
    appFont.setPixelSize(14);
    app.setFont(appFont);

    logToFile("App created. Registering types...");

    // Register C++ types for QML
    qmlRegisterType<DefenderEngine>("zack.defender", 1, 0, "DefenderEngine");
    qmlRegisterType<HashEngine>("zack.defender", 1, 0, "HashEngine");
    qmlRegisterType<ClamAVEngine>("zack.defender", 1, 0, "ClamAVEngine");
    qmlRegisterType<YaraEngine>("zack.defender", 1, 0, "YaraEngine");
    qmlRegisterType<EngineManager>("zack.defender", 1, 0, "EngineManager");
    qmlRegisterType<ProtectionMonitor>("zack.defender", 1, 0, "ProtectionMonitor");
    qmlRegisterType<ThreatHistory>("zack.defender", 1, 0, "ThreatHistory");
    qmlRegisterType<QuarantineManager>("zack.defender", 1, 0, "QuarantineManager");
    qmlRegisterType<ThemeManager>("zack.defender", 1, 0, "ThemeManager");
    qmlRegisterType<UpdateManager>("zack.defender", 1, 0, "UpdateManager");
    qmlRegisterType<ContextMenuManager>("zack.defender", 1, 0, "ContextMenuManager");
    qmlRegisterType<SilverFoxEngine>("zack.defender", 1, 0, "SilverFoxEngine");
    qRegisterMetaType<ThreatEntry>("ThreatEntry");

    logToFile("Types registered. Setting up engine...");

    QQmlApplicationEngine engine;

    // Add import paths for md3.Core
    QString appDir = QCoreApplication::applicationDirPath();
    logToFile("App dir: " + appDir);

    engine.addImportPath(appDir + "/qml");
    engine.addImportPath("qrc:/");

    logToFile("Import paths added. Loading QML...");

    // ── System tray ──────────────────────────────────────────
    TrayManager *trayManager = new TrayManager(&app);
    QObject::connect(trayManager, &TrayManager::exitRequested, &app, &QCoreApplication::quit);
    engine.rootContext()->setContextProperty("trayManager", trayManager);

    // ── Right-click scan: pass startup path to QML ──────────
    engine.rootContext()->setContextProperty("_startupScanPath", startupScanPath);

    // ── Right-click context menu manager ────────────────────
    ContextMenuManager *ctxMenuManager = new ContextMenuManager(&app);
    engine.rootContext()->setContextProperty("ctxMenuManager", ctxMenuManager);

    const QUrl url("qrc:/zack/defender/src/qml/Main.qml");
    logToFile("URL: " + url.toString());

    // Capture QML warnings/errors
    QObject::connect(&engine, &QQmlApplicationEngine::warnings, &app,
        [](const QList<QQmlError> &warnings) {
            for (const auto &w : warnings) {
                logToFile("QML WARNING: " + w.toString());
            }
        });

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app,
        []() {
            logToFile("FATAL: QML object creation failed!");
            QCoreApplication::exit(-1);
        }, Qt::QueuedConnection);

    engine.load(url);

    if (engine.rootObjects().isEmpty()) {
        logToFile("FATAL: No root objects! (QQmlApplicationEngine returned empty)");
        g_logFile.close();
        return -1;
    }

    // Setup tray — needs the root window alive
    trayManager->setup(engine.rootObjects().first());

    logToFile("SUCCESS! Window created. Entering event loop.");
    g_logFile.close();
    return app.exec();
}

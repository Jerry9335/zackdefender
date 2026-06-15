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
#include "backend/DefenderScanner.h"
#include "backend/ProtectionMonitor.h"
#include "backend/ThreatHistory.h"
#include "backend/QuarantineManager.h"
#include "backend/TrayManager.h"
#include "backend/ThemeManager.h"
#include "backend/UpdateManager.h"

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
            logToFile("Another instance detected — showing notice and exiting");
            QMessageBox::information(nullptr, "Zack Defender",
                "Zack Defender 已在运行中 🛡\n\n"
                "程序已在系统托盘中运行，请查看任务栏右侧的托盘图标。\n"
                "如需重新打开窗口，请双击托盘图标。");
        }
        g_logFile.close();
        return 0;
    }

    // ── Global font: Microsoft YaHei ─────────────────────────────
    QFont appFont = app.font();
    appFont.setFamily("Microsoft YaHei");
    appFont.setPixelSize(14);
    app.setFont(appFont);

    logToFile("App created. Registering types...");

    // Register C++ types for QML
    qmlRegisterType<DefenderScanner>("zack.defender", 1, 0, "DefenderScanner");
    qmlRegisterType<ProtectionMonitor>("zack.defender", 1, 0, "ProtectionMonitor");
    qmlRegisterType<ThreatHistory>("zack.defender", 1, 0, "ThreatHistory");
    qmlRegisterType<QuarantineManager>("zack.defender", 1, 0, "QuarantineManager");
    qmlRegisterType<ThemeManager>("zack.defender", 1, 0, "ThemeManager");
    qmlRegisterType<UpdateManager>("zack.defender", 1, 0, "UpdateManager");
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

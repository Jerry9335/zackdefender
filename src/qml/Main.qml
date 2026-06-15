import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import md3.Core
import zack.defender

ApplicationWindow {
    id: mainWindow
    visible: trayManager.windowVisible
    width: 960
    height: 680
    minimumWidth: 780
    minimumHeight: 520
    title: "Zack Defender - 安全中心"
    color: Theme.color.background

    // ── Close → minimize to tray ────────────────────────
    onClosing: (close) => {
        close.accepted = false
        trayManager.hideWindow()
    }

    // ── Global scanner — survives page transitions ─────────────
    property alias scanner: globalScanner
    DefenderScanner { id: globalScanner }

    // ── Global monitors — survives page transitions ────────────
    property alias monitor: globalMonitor
    ProtectionMonitor {
        id: globalMonitor
        Component.onCompleted: refresh()
    }

    property alias history: globalHistory
    ThreatHistory {
        id: globalHistory
        Component.onCompleted: refresh()
    }

    property alias quarantine: globalQuarantine
    QuarantineManager {
        id: globalQuarantine
        Component.onCompleted: refresh()
    }

    // ── Theme manager — survives page transitions ───────────────
    property alias themeManager: globalThemeManager
    ThemeManager {
        id: globalThemeManager
        Component.onCompleted: {
            // Initial theme sync
            StyleManager.isDarkTheme = globalThemeManager.effectiveDark
        }
    }

    // ── Sync theme to MD3 StyleManager ──────────────────────────
    Connections {
        target: globalThemeManager
        function onEffectiveDarkChanged() {
            StyleManager.isDarkTheme = globalThemeManager.effectiveDark
        }
    }

    // ── Update / changelog ─────────────────────────────────────
    UpdateManager { id: updateManager }

    // Show changelog on first run after update
    Component.onCompleted: {
        if (updateManager.showChangelog) {
            changelogDialog.open()
        }
    }

    function showChangelog() {
        changelogDialog.open()
    }

    // ── Navigation state ───────────────────────────────────────
    property var currentItem: navItems[0]
    property int pendingScanType: -1
    property string pendingCustomPath: ""
    property string pendingPage: ""

    // ── Smooth page switching ──────────────────────────────────
    function switchPage(page, sidebarItem) {
        if (pageLoader.source.toString().endsWith(page)) return

        // Update sidebar highlight
        if (sidebarItem) {
            mainWindow.currentItem = sidebarItem
            navDrawer.currentItem = sidebarItem
        }

        // Fade out → change source → fade in
        pendingPage = page
        contentArea.opacity = 0
        transitionTimer.restart()
    }

    function goToDashboard() {
        switchPage("pages/DashboardPage.qml", navItems[0])
    }

    function navigateToScan(scanType, customPath) {
        pendingScanType = scanType
        pendingCustomPath = customPath || ""
        switchPage("pages/ScanPage.qml", navItems[1])
    }

    // Open folder picker for custom scan
    function startCustomScan() {
        folderDialog.open()
    }

    // ── Folder picker ───────────────────────────────────────
    FolderDialog {
        id: folderDialog
        title: "选择扫描目标文件夹"
        onAccepted: {
            var raw = selectedFolder.toString()
            var path = raw.replace(/^file:\/\/\//, "")
            pendingScanType = 3
            pendingCustomPath = path
            if (pageLoader.source.toString().endsWith("pages/ScanPage.qml")) {
                // Already on scan page — call triggerScan directly
                pageLoader.item.triggerScan(3, path)
            } else {
                switchPage("pages/ScanPage.qml", navItems[1])
            }
        }
    }

    Timer {
        id: transitionTimer
        interval: 160  // match fade-out duration + small buffer
        onTriggered: {
            pageLoader.source = pendingPage
            contentArea.opacity = 1
        }
    }

    // ── Nav items (order matters — index used above) ───────────
    property var navItems: [
        { type: "item", text: "仪表盘", icon: "dashboard", page: "pages/DashboardPage.qml" },
        { type: "item", text: "病毒扫描", icon: "search", page: "pages/ScanPage.qml" },
        { type: "divider" },
        { type: "item", text: "实时保护", icon: "shield", page: "pages/ProtectionPage.qml" },
        { type: "item", text: "隔离区", icon: "archive", page: "pages/QuarantinePage.qml" },
        { type: "divider" },
        { type: "item", text: "设置 & 历史", icon: "settings", page: "pages/SettingsPage.qml" }
    ]

    RowLayout {
        anchors.fill: parent
        spacing: 0

        NavigationDrawer {
            id: navDrawer
            modal: false
            drawerWidth: 260
            title: "Zack Defender"
            model: mainWindow.navItems
            currentItem: mainWindow.currentItem
            onItemClicked: (itemData) => {
                mainWindow.currentItem = itemData
                navDrawer.currentItem = itemData
                switchPage(itemData.page, itemData)
            }
        }

        Rectangle {
            id: contentArea
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: Theme.color.background
            clip: true
            opacity: 1

            // ── Page transition animation ─────────────────────
            Behavior on opacity {
                NumberAnimation {
                    duration: 150
                    easing.type: Easing.InOutQuad
                }
            }

            Loader {
                id: pageLoader
                anchors.fill: parent
                anchors.margins: 24
                source: "pages/DashboardPage.qml"
            }
        }
    }

    // ════════════════════════════════════════════════════════════
    // ── Global Snackbar (overlays everything) ──────────────────
    // ════════════════════════════════════════════════════════════
    property var pendingSnackAction: null

    function showSnackbar(message, actionText, actionCallback) {
        pendingSnackAction = actionCallback || null
        globalSnackbar.text = message
        globalSnackbar.actionText = actionText || ""
        globalSnackbar.open()
    }

    Snackbar {
        id: globalSnackbar
        z: 999
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottomMargin: 24
        timeout: 5000

        onActionClicked: {
            if (mainWindow.pendingSnackAction) {
                mainWindow.pendingSnackAction()
                mainWindow.pendingSnackAction = null
            }
        }
    }

    // ── Auto-notifications from scanner ─────────────────────────
    Connections {
        target: globalScanner
        function onScanFinished(exitCode, threatsFound) {
            if (exitCode === -1) return  // cancelled
            if (threatsFound === 0) {
                showSnackbar("✅ 扫描完成 — 未发现威胁", "", null)
                trayManager.showNotification("扫描完成 ✅", "未发现威胁", 1)
            } else {
                showSnackbar("⚠️ 发现 " + threatsFound + " 个威胁！", "查看详情",
                    function() { navigateToScan(-1, "") })
                trayManager.showNotification("⚠️ 发现威胁",
                    "发现 " + threatsFound + " 个威胁！", 2)
            }
        }
        function onThreatFound(filePath, threatName) {
            var name = threatName || "未知威胁"
            var shortName = name.length > 40 ? name.substring(0, 40) + "..." : name
            showSnackbar("⚠️ 检测到: " + shortName, "", null)
        }
    }

    // ── Tray actions ────────────────────────────────────────────
    Connections {
        target: trayManager
        function onShowWindowRequested() {
            mainWindow.show()
            mainWindow.raise()
            mainWindow.requestActivate()
        }
        function onQuickScanRequested() {
            mainWindow.show()
            mainWindow.raise()
            mainWindow.requestActivate()
            navigateToScan(1, "")
        }
    }

    // ════════════════════════════════════════════════════════════
    // ── Changelog Dialog ────────────────────────────────────────
    // ════════════════════════════════════════════════════════════
    Dialog {
        id: changelogDialog
        title: "📋 更新日志 — Zack Defender v1.2.0"
        width: 520
        height: 420
        acceptText: "知道了"
        showRejectButton: false

        onAccepted: updateManager.dismiss()

        content: ColumnLayout {
            spacing: 12
            width: parent.width

            Text {
                text: "v1.2.0 — " + new Date().toLocaleDateString()
                font.pixelSize: 16; font.bold: true
                color: Theme.color.onSurfaceColor
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: Theme.color.outlineVariant
            }

            ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true

                ColumnLayout {
                    spacing: 10

                    Repeater {
                        model: [
                            { emoji: "🛡", text: "扫描威胁处理联动：扫描发现威胁后可一键隔离或忽略" },
                            { emoji: "🔔", text: "通知提醒系统：Snackbar 应用内通知 + 托盘气泡通知" },
                            { emoji: "📌", text: "系统托盘图标：最小化到托盘、右键菜单快速扫描" },
                            { emoji: "🎨", text: "深色/浅色主题：支持跟随系统、手动切换深浅色模式" },
                            { emoji: "📋", text: "更新日志弹窗：首次启动显示更新内容（就是你正在看的这个！）" }
                        ]
                        delegate: RowLayout {
                            spacing: 10
                            Layout.fillWidth: true
                            Text {
                                text: modelData.emoji
                                font.pixelSize: 18
                            }
                            Text {
                                text: modelData.text
                                font.pixelSize: 14
                                color: Theme.color.onSurfaceColor
                                Layout.fillWidth: true
                                wrapMode: Text.WordWrap
                            }
                        }
                    }
                }
            }
        }
    }
}

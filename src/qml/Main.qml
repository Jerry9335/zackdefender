import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import md3.Core
import zack.defender

ApplicationWindow {
   id: mainWindow
   visible: trayManager.windowVisible
   width: 1100
   height: 780
   minimumWidth: 1100
   minimumHeight: 780
   maximumWidth: 1100
   maximumHeight: 780
   title: "Zack Defender - 安全中心"
   color: Theme.color.background

   // ── Close → minimize to tray ────────────────────────
   onClosing: (close) => {
       close.accepted = false
       // Delay hide — can't modify visibility during close event
       hideToTrayTimer.start()
   }

   Timer {
       id: hideToTrayTimer
       interval: 0  // next event loop iteration
       onTriggered: trayManager.hideWindow()
   }

   // ── Global engine manager — survives page transitions ──
   property alias engineManager: globalEngineManager
   EngineManager { id: globalEngineManager }

   // ── Global threat history — persists across scans ──────────
   property alias threatHistory: globalThreatHistory
   ThreatHistory { id: globalThreatHistory }

   // ── Connect engine threats to global history ───────────────
   Connections {
       target: globalEngineManager
       function onThreatFound(filePath, threatName) {
           globalThreatHistory.addThreat(filePath, threatName, "Zack Defender", 3)
       }
   }

   // ── Theme manager — survives page transitions ───────────────
   property alias themeManager: globalThemeManager
   ThemeManager {
       id: globalThemeManager
       Component.onCompleted: {
           // Initial theme sync
           StyleManager.isDarkTheme = globalThemeManager.effectiveDark
           // Restore saved seed color
           if (globalThemeManager.customSeedColor.a > 0) {
               StyleManager.seedColor = globalThemeManager.customSeedColor
           }
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
   property alias updateManager: updateManager
   UpdateManager { id: updateManager }

   // Show changelog on first run after update
   Component.onCompleted: {
       if (updateManager.showChangelog) {
           changelogDialog.open()
       }
       // ── Right-click scan: auto-start scan on launch ─────
       if (typeof _startupScanPath !== "undefined" && _startupScanPath !== "") {
           navigateToScan(3, _startupScanPath)
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
       // If already on scan page, trigger directly (don't reload)
       if (pageLoader.source.toString().endsWith("pages/ScanPage.qml")) {
           mainWindow.currentItem = navItems[1]
           navDrawer.currentItem = navItems[1]
           if (pageLoader.item && pageLoader.item.triggerScan) {
               pageLoader.item.triggerScan(scanType, customPath || "")
           }
       } else {
           switchPage("pages/ScanPage.qml", navItems[1])
       }
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

   // ── Right-click scan: poll for requests from other instances ──
   Timer {
       id: scanRequestTimer
       interval: 1500
       running: true
       repeat: true
       onTriggered: {
           var path = trayManager.checkScanRequest()
           if (path && path !== "") {
               // Use trayManager.showWindow() — keeps C++ state in sync
               trayManager.showWindow()
               navigateToScan(3, path)
           }
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
       { type: "item", text: "银狐解救 (Beta)", icon: "bug_report", page: "pages/SilverFoxPage.qml" },
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
       target: globalEngineManager
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
       title: "📋 更新日志 — Zack Defender v1.9.0"
       width: 520
       height: 540
       acceptText: "知道了"
       showRejectButton: false

       onAccepted: updateManager.dismiss()

       content: ColumnLayout {
           spacing: 12
           width: parent.width

           Text {
               text: "v1.9.0 — " + new Date().toLocaleDateString()
               font.pixelSize: 16; font.bold: true
               color: Theme.color.onSurfaceColor
           }
           Text {
               text: "（为什么跳过 v1.8？它活在 dev 分支里，功能迭代太快没来得及发正式版 😅）"
               font.pixelSize: 12; color: Theme.color.onSurfaceVariantColor
               wrapMode: Text.WordWrap; Layout.fillWidth: true
           }

           Rectangle {
               Layout.fillWidth: true
               height: 1
               color: Theme.color.outlineVariant
           }

           ScrollView {
               Layout.fillWidth: true
               Layout.preferredHeight: 380
               clip: true

               ColumnLayout {
                   spacing: 12

                   // ── v1.9.0 section ──
                   Text {
                       text: "🆕 v1.9.0 — 视觉焕新 & 体验升级（正式版，首个 MSIX 安装包）"
                       font.pixelSize: 14; font.bold: true
                       color: Theme.color.primary
                   }

                   Repeater {
                       model: [
                           { emoji: "📦", text: "本软件基于 Qt 6.11 构建，要求 Windows 10 21H2 或更高版本。从此版本起，安装包采用 MSIX 格式，提供更安全的应用打包和自动更新体验！" },
                           { emoji: "🎨", text: "全新吉祥物系统：仪表盘、银狐检测、扫描结果页全部换上专属图片，告别单调 emoji" },
                           { emoji: "🖼", text: "扫描结果图标重设计：安全/危险状态用图片直观展示，去掉圆形底色，更加清爽现代" },
                           { emoji: "📐", text: "UI 尺寸精细调优：多处卡片高度、图片尺寸统一放大，布局更加饱满协调" },
                           { emoji: "🦊", text: "银狐检测卡片新增专属图标，视觉辨识度提升" },
                           { emoji: "🔧", text: "修复 qrc 资源路径问题，确保图片稳定加载不丢失" }
                       ]
                       delegate: RowLayout {
                           spacing: 10
                           Layout.fillWidth: true
                           Text { text: modelData.emoji; font.pixelSize: 18 }
                           Text {
                               text: modelData.text
                               font.pixelSize: 14
                               color: Theme.color.onSurfaceColor
                               Layout.fillWidth: true
                               wrapMode: Text.WordWrap
                           }
                       }
                   }

                   Rectangle { Layout.fillWidth: true; height: 1; color: Theme.color.outlineVariant }

                   // ── v1.8.0 section ──
                   Text {
                       text: "🏗 v1.8.0 — 界面重构（内部版本，未正式发布）"
                       font.pixelSize: 14; font.bold: true
                       color: Theme.color.primary
                   }

                   Repeater {
                       model: [
                           { emoji: "📋", text: "全面重写仪表盘布局，卡片式设计语言统一，信息层级更清晰" },
                           { emoji: "⚡", text: "进度指示器增强：波浪动画更流畅，加载状态视觉效果大幅提升" },
                           { emoji: "🔧", text: "修复 20+ QML 布局问题：RowLayout 塌缩、Card 高度裁剪、Image 显示异常等" },
                           { emoji: "🎯", text: "实时保护监视器重构，状态检测更准确，启动速度更快" }
                       ]
                       delegate: RowLayout {
                           spacing: 10
                           Layout.fillWidth: true
                           Text { text: modelData.emoji; font.pixelSize: 18 }
                           Text {
                               text: modelData.text
                               font.pixelSize: 14
                               color: Theme.color.onSurfaceColor
                               Layout.fillWidth: true
                               wrapMode: Text.WordWrap
                           }
                       }
                   }

                   Rectangle { Layout.fillWidth: true; height: 1; color: Theme.color.outlineVariant }

                   // ── v1.7.0 section ──
                   Text {
                       text: "🆕 v1.7.0 — 引擎替换（内部版本，未正式发布）"
                       font.pixelSize: 14; font.bold: true
                       color: Theme.color.primary
                   }

                   Repeater {
                       model: [
                           { emoji: "🦀", text: "新增 ClamAV 本地引擎：开源反病毒引擎，支持 20+ 压缩格式，多线程扫描，离线可用" },
                           { emoji: "🎯", text: "新增 YARA 规则引擎：基于模式匹配的恶意软件指纹检测，内置勒索软件/加壳/可疑脚本规则" },
                           { emoji: "🗑", text: "移除 VirusTotal 云引擎：免费 API 仅 500 次/天，限制严重；换为本地引擎，无限制扫描" },
                           { emoji: "⚡", text: "新扫描顺序：哈希 → ClamAV → YARA → Windows Defender，由快到深，层层递进" }
                       ]
                       delegate: RowLayout {
                           spacing: 10
                           Layout.fillWidth: true
                           Text { text: modelData.emoji; font.pixelSize: 18 }
                           Text {
                               text: modelData.text
                               font.pixelSize: 14
                               color: Theme.color.onSurfaceColor
                               Layout.fillWidth: true
                               wrapMode: Text.WordWrap
                           }
                       }
                   }

                   Rectangle { Layout.fillWidth: true; height: 1; color: Theme.color.outlineVariant }

                   // ── v1.6.0 section ──
                   Text {
                       text: "🦊 v1.6.0 — 银狐病毒专杀（内部版本，未正式发布）"
                       font.pixelSize: 14; font.bold: true
                       color: Theme.color.primary
                   }

                   Repeater {
                       model: [
                           { emoji: "🔍", text: "SilverFoxEngine：4 阶段检测（隐藏进程 → 启动项 → 可疑服务 → 已知特征）+ 4 阶段清理" },
                           { emoji: "🛡", text: "独立 SilverFoxPage 页面，一键扫描 + 清理银狐病毒" },
                           { emoji: "🔐", text: "安全设计：清理逻辑分离到外部脚本，避免杀软误报自身程序" }
                       ]
                       delegate: RowLayout {
                           spacing: 10
                           Layout.fillWidth: true
                           Text { text: modelData.emoji; font.pixelSize: 18 }
                           Text {
                               text: modelData.text
                               font.pixelSize: 14
                               color: Theme.color.onSurfaceColor
                               Layout.fillWidth: true
                               wrapMode: Text.WordWrap
                           }
                       }
                   }

                   Rectangle { Layout.fillWidth: true; height: 1; color: Theme.color.outlineVariant }

                   // ── v1.5.0 section ──
                   Text {
                       text: "🎨 v1.5.0 — 扫描重构 & UI 升级（内部版本，未正式发布）"
                       font.pixelSize: 14; font.bold: true
                       color: Theme.color.primary
                   }

                   Repeater {
                       model: [
                           { emoji: "⚡", text: "扫描引擎从并行改为顺序执行，UI 不再卡死" },
                           { emoji: "🎨", text: "MD3 动态 blob 加载指示器 + 波浪进度条 + 粒子背景动画" },
                           { emoji: "📋", text: "哈希黑名单从 8 条扩展到 52 条（18 MD5 + 16 SHA256 + 18 可疑文件名）" },
                           { emoji: "🔧", text: "修复一堆 MD3 QML 坑：RowLayout/Card/进度条/elide 等布局问题" }
                       ]
                       delegate: RowLayout {
                           spacing: 10
                           Layout.fillWidth: true
                           Text { text: modelData.emoji; font.pixelSize: 18 }
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

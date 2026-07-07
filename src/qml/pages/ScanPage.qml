import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import md3.Core
import zack.defender

Item {
   id: root

   // ── Entrance animation ──────────────────────────────────
   opacity: 0; y: 12
   Behavior on opacity { NumberAnimation { duration: 200; easing.type: Easing.OutCubic } }
   Behavior on y { NumberAnimation { duration: 250; easing.type: Easing.OutCubic } }

   QuarantineManager { id: quarantineManager }

   property int lastExitCode: -1
   property int lastThreatsFound: 0
   property bool scanComplete: false
   property bool scanCancelled: false

   // Called by Main.qml when custom scan is triggered while page is loaded
   function triggerScan(scanType, customPath) {
       scanComplete = false
       scanCancelled = false
       mainWindow.engineManager.startScan(scanType, customPath)
   }

   Component.onCompleted: {
       // Entrance animation
       opacity = 1; y = 0
       // Trigger pending scan from navigation bridge
       if (typeof mainWindow !== "undefined" && mainWindow.pendingScanType > 0) {
           var st = mainWindow.pendingScanType
           var cp = mainWindow.pendingCustomPath
           mainWindow.pendingScanType = -1
           mainWindow.pendingCustomPath = ""
           scanComplete = false
           mainWindow.engineManager.startScan(st, cp)
       }
   }

   // ── Animated background — runs during scanning ──────────────
   IndexBackground {
       anchors.fill: parent
       running: mainWindow.engineManager.scanning
       z: -1
   }

   ColumnLayout {
       anchors.fill: parent
       spacing: 20

       // Header
       RowLayout {
           Text {
               text: "病毒扫描"
               font.pixelSize: 28; font.bold: true
               color: Theme.color.onSurfaceColor
           }
           Item { Layout.fillWidth: true }
           Rectangle {
               visible: mainWindow.engineManager.scanning && mainWindow.engineManager.progress < 100
               radius: 8; color: "#1976D2"
               implicitWidth: badgeText.width + 24; implicitHeight: 28
               Text {
                   id: badgeText
                   anchors.centerIn: parent
                   text: "扫描中..."; font.pixelSize: 12; color: "white"
               }
           }
           Button {
               text: "回到仪表盘"
               onClicked: mainWindow.goToDashboard()
           }
       }

       // Scan controls
       Card {
           Layout.fillWidth: true; padding: 24
           implicitHeight: 280

           ColumnLayout {
               spacing: 16
               Text {
                   text: mainWindow.engineManager.scanning ? "正在扫描中..." : "选择扫描类型"
                   font.pixelSize: 18; font.bold: true
                   color: Theme.color.onSurfaceColor
               }
               RowLayout {
                   spacing: 10
                   Button {
                       text: "⚡ 快速扫描"; enabled: !mainWindow.engineManager.scanning
                       Layout.preferredHeight: 42
                       onClicked: { scanComplete = false; mainWindow.engineManager.startScan(1) }
                   }
                   Button {
                       text: "🔎 全面扫描"; enabled: !mainWindow.engineManager.scanning
                       Layout.preferredHeight: 42
                       onClicked: { scanComplete = false; mainWindow.engineManager.startScan(2) }
                   }
                   Button {
                       text: "📁 自定义"; enabled: !mainWindow.engineManager.scanning
                       Layout.preferredHeight: 42
                       onClicked: mainWindow.startCustomScan()
                   }
                   Button {
                       text: "⏹ 取消"; visible: mainWindow.engineManager.scanning
                       Layout.preferredHeight: 42
                       onClicked: mainWindow.engineManager.cancelScan()
                   }
               }

               // Progress indicator
               ColumnLayout {
                   visible: mainWindow.engineManager.scanning && mainWindow.engineManager.progress < 100
                   spacing: 14
                   Layout.fillWidth: true
                   Layout.maximumWidth: 520
                   Layout.alignment: Qt.AlignHCenter

                   // Row 1: loading spinner + percentage
                   RowLayout {
                       spacing: 14
                       Layout.fillWidth: true

                       LoadingIndicator {
                           id: loadingSpinner
                           Layout.preferredWidth: 40
                           Layout.preferredHeight: 40
                           running: true
                           size: 40
                       }

                       Text {
                           text: mainWindow.engineManager.progress + "%"
                           font.pixelSize: 22; font.bold: true
                           color: Theme.color.onSurfaceColor
                       }

                       Item { Layout.fillWidth: true }

                       // Engine badge
                       Rectangle {
                           radius: 6
                           color: Theme.color.primaryContainer
                           implicitWidth: engineBadgeText.implicitWidth + 16
                           implicitHeight: 24
                           Text {
                               id: engineBadgeText
                               anchors.centerIn: parent
                               text: {
                                   var cf = mainWindow.engineManager.currentFile || ""
                                   if (cf.startsWith("[哈希检测")) return "🔍 哈希"
                                   if (cf.startsWith("[ClamAV")) return "🦀 ClamAV"
                                   if (cf.startsWith("[YARA")) return "🎯 YARA"
                                   if (cf.startsWith("[Windows Defender")) return "🛡 Defender"
                                   return "⏳ 扫描中"
                               }
                               font.pixelSize: 11
                               color: Theme.color.onPrimaryContainer
                           }
                       }
                   }

                   // Row 2: current file path
                   Text {
                       text: mainWindow.engineManager.currentFile || "正在初始化..."
                       font.pixelSize: 12
                       color: Theme.color.onSurfaceVariantColor
                       elide: Text.ElideMiddle
                       Layout.fillWidth: true
                   }

                   // Row 3: Animated wavy bar (visual+active indicator)
                   LinearProgress {
                       Layout.fillWidth: true
                       wavy: true
                       indeterminate: mainWindow.engineManager.progress < 100
                       value: mainWindow.engineManager.progress >= 100 ? 1.0 : 0.0
                   }

                   // Row 3b: Real determinate progress (thin bar)
                   Rectangle {
                       Layout.fillWidth: true
                       implicitHeight: 6
                       radius: 3
                       color: Theme.color.surfaceVariant
                       Rectangle {
                           width: parent.width * (mainWindow.engineManager.progress / 100.0)
                           height: parent.height
                           radius: 3
                           color: Theme.color.primary
                           Behavior on width { NumberAnimation { duration: 180; easing.type: Easing.OutCubic } }
                       }
                   }
               }
           }
       }

       // Results summary
       Card {
           Layout.fillWidth: true; padding: 20
           implicitHeight: 120
           visible: scanComplete || scanCancelled
           RowLayout {
               spacing: 20
               Image {
                   visible: !scanCancelled
                   source: lastThreatsFound === 0
                       ? "qrc:/assets/scan_safe.png"
                       : "qrc:/assets/scan_danger.png"
                   fillMode: Image.PreserveAspectFit
                   Layout.preferredWidth: 80
                   Layout.preferredHeight: 80
               }
               Rectangle {
                   visible: scanCancelled
                   width: 56; height: 56; radius: 28
                   color: "#FF9800"
                   Text {
                       anchors.centerIn: parent
                       text: "✕"
                       font.pixelSize: 28; color: "white"
                   }
               }
               ColumnLayout {
                   spacing: 4
                   Text {
                       text: scanCancelled ? "扫描已取消"
                           : (lastThreatsFound === 0 ? "扫描完成 — 未发现威胁"
                           : "扫描完成 — 发现 " + lastThreatsFound + " 个威胁")
                       font.pixelSize: 18; font.bold: true
                       color: Theme.color.onSurfaceColor
                   }
               }
           }
       }

       // ── Threat handling ───────────────────────────────────────
       Card {
           Layout.fillWidth: true; padding: 20
           implicitHeight: 320
           visible: scanComplete && !scanCancelled
                    && mainWindow.engineManager.threats
                    && mainWindow.engineManager.threats.length > 0
           color: "#FFF5F5"

           ColumnLayout {
               anchors.fill: parent
               spacing: 14

               // Header row
               RowLayout {
                   spacing: 12
                   Text {
                       text: "⚠ 发现 " + mainWindow.engineManager.threats.length + " 个威胁"
                       font.pixelSize: 18; font.bold: true
                       color: "#C62828"
                   }
                   Item { Layout.fillWidth: true }
                   Button {
                       text: "🛡 全部隔离"
                       onClicked: {
                           var list = mainWindow.engineManager.threats
                           for (var i = 0; i < list.length; i++) {
                               quarantineManager.quarantineFile(
                                   list[i].filePath, list[i].threatName)
                           }
                           var count = list.length
                           mainWindow.engineManager.clearThreats()
                           mainWindow.showSnackbar("🛡 已隔离 " + count + " 个文件", "", null)
                       }
                   }
                   Button {
                       text: "✕ 全部忽略"
                       onClicked: {
                           var count = mainWindow.engineManager.threats.length
                           mainWindow.engineManager.clearThreats()
                           mainWindow.showSnackbar("已忽略 " + count + " 个威胁", "", null)
                       }
                   }
               }

               // Individual threat items (scrollable)
               ScrollView {
                   Layout.fillWidth: true
                   Layout.fillHeight: true
                   clip: true

                   Column {
                       spacing: 8
                       width: parent.width

                   Repeater {
                       model: mainWindow.engineManager.threats
                       delegate: Rectangle {
                           width: parent.width
                           height: threatRow.implicitHeight + 24
                           radius: 8
                           color: "#FFEBEE"
                           border { width: 1; color: "#FFCDD2" }

                           RowLayout {
                               id: threatRow
                               anchors.fill: parent
                               anchors.margins: 12
                               spacing: 12

                               ColumnLayout {
                                   spacing: 4
                                   Layout.fillWidth: true
                                   Text {
                                       text: modelData.threatName || "未知威胁"
                                       font.pixelSize: 14; font.bold: true
                                       color: Theme.color.onSurfaceColor
                                       Layout.fillWidth: true
                                       elide: Text.ElideRight
                                   }
                                   Text {
                                       text: "路径: " + (modelData.filePath || "-")
                                       font.pixelSize: 12
                                       color: Theme.color.onSurfaceVariantColor
                                       Layout.fillWidth: true
                                       elide: Text.ElideMiddle
                                   }
                               }

                               Button {
                                   text: "🛡 隔离"
                                   implicitHeight: 32
                                   onClicked: {
                                       quarantineManager.quarantineFile(
                                           modelData.filePath, modelData.threatName)
                                       var name = modelData.threatName || "未知威胁"
                                       var shortName = name.length > 30 ? name.substring(0, 30) + "..." : name
                                       mainWindow.engineManager.removeThreat(modelData.filePath)
                                       mainWindow.showSnackbar("🛡 已隔离: " + shortName, "", null)
                                   }
                               }
                               Button {
                                   text: "忽略"
                                   implicitHeight: 32
                                   onClicked: mainWindow.engineManager.removeThreat(modelData.filePath)
                               }
                           }   // RowLayout (threatRow)
                       }       // Rectangle (delegate)
                   }           // Repeater
               }               // Column
           }                   // ScrollView
           }                   // ColumnLayout (card)
       }                       // Card

       // Scan log
       Card {
           Layout.fillWidth: true; Layout.fillHeight: true; padding: 16

           ColumnLayout {
               anchors.fill: parent; spacing: 10
               RowLayout {
                   Text {
                       text: "扫描日志"
                       font.pixelSize: 16; font.bold: true
                       color: Theme.color.onSurfaceColor
                   }
                   Item { Layout.fillWidth: true }
                   Text {
                       text: mainWindow.engineManager.scanLog.length + " 条"
                       font.pixelSize: 12; color: Theme.color.onSurfaceVariantColor
                   }
               }
               ScrollView {
                   Layout.fillWidth: true; Layout.fillHeight: true; clip: true
                   ListView {
                       id: outputList
                       model: mainWindow.engineManager.scanLog
                       delegate: Text {
                           width: outputList.width - 16
                           text: modelData.text; font.pixelSize: 13
                           color: modelData.isThreat ? "#F44336"
                                : modelData.text.includes("完成") ? "#4CAF50"
                                : Theme.color.onSurfaceVariantColor
                           wrapMode: Text.Wrap
                           font.family: "Consolas, monospace"
                       }
                   }
               }
           }
       }
   }

   // ── Connections to global scanner ──────────────────────
   Connections {
       target: mainWindow.engineManager
       function onScanLogChanged() {
           outputList.positionViewAtEnd()
       }
       function onScanFinished(exitCode, threatsFound) {
           outputList.positionViewAtEnd()
           if (exitCode === -1) {
               scanCancelled = true
               scanComplete = false
           } else {
               lastExitCode = exitCode
               lastThreatsFound = threatsFound
               scanComplete = true
               scanCancelled = false
           }
       }
   }

   Connections {
       target: mainWindow.engineManager
       function onScanningChanged() {
           if (mainWindow.engineManager.scanning) {
               scanComplete = false
               scanCancelled = false
           }
       }
   }
}

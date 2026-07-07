import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import md3.Core
import zack.defender

Item {
   id: root

   // ── Entrance animation ──────────────────────────────────
   opacity: 0; y: 12
   Component.onCompleted: { opacity = 1; y = 0 }
   Behavior on opacity { NumberAnimation { duration: 200; easing.type: Easing.OutCubic } }
   Behavior on y { NumberAnimation { duration: 250; easing.type: Easing.OutCubic } }

   ProtectionMonitor { id: monitor; Component.onCompleted: refresh() }
   

   // ── Animated background — always running ───────────────────
   IndexBackground {
       anchors.fill: parent
       running: true
       z: -1
   }

   Flickable {
       anchors.fill: parent
       contentHeight: contentLayout.implicitHeight
       clip: true

       ColumnLayout {
           id: contentLayout
           width: parent.width
           spacing: 20

           // ── Ongoing scan mini-card ─────────────────────────
           Card {
               Layout.fillWidth: true; padding: 16
               visible: mainWindow.engineManager.scanning
               color: "#E3F2FD"

               RowLayout {
                   spacing: 16
                   Rectangle {
                       width: 40; height: 40; radius: 20; color: "#1976D2"
                       Text {
                           anchors.centerIn: parent
                           text: "🔍"; font.pixelSize: 18
                       }
                   }
                   ColumnLayout {
                       spacing: 2
                       Text {
                           text: "正在后台扫描..."
                           font.pixelSize: 16; font.bold: true
                           color: Theme.color.onSurfaceColor
                       }
                       Text {
                           text: mainWindow.engineManager.progress + "% — "
                               + (mainWindow.engineManager.currentFile || "扫描中...")
                           font.pixelSize: 12; color: Theme.color.onSurfaceVariantColor
                           elide: Text.ElideMiddle
                           Layout.maximumWidth: 350
                       }
                   }
                   Item { Layout.fillWidth: true }
                   Button {
                       text: "📋 查看详情"
                       onClicked: mainWindow.navigateToScan(-1, "")
                   }
                   Button {
                       text: "⏹ 取消扫描"
                       onClicked: mainWindow.engineManager.cancelScan()
                   }
               }
           }

           // ── Header ──────────────────────────────────────────
           RowLayout {
               spacing: 16
               Text {
                   text: "Zack Defender"
                   font.pixelSize: 28; font.bold: true
                   color: Theme.color.onSurfaceColor
               }
               Item { Layout.fillWidth: true }
               Rectangle {
                   radius: 8
                   color: monitor.realTimeEnabled ? "#4CAF50" : "#F44336"
                   implicitWidth: statusChipText.width + 24
                   implicitHeight: 28
                   Text {
                       id: statusChipText
                       anchors.centerIn: parent
                       text: monitor.realTimeEnabled ? "🛡 已保护" : "⚠ 有风险"
                       font.pixelSize: 12; color: "white"
                   }
               }
               Button {
                   text: "🔄 刷新"
                   onClicked: { monitor.refresh(); mainWindow.threatHistory.refresh() }
               }
           }

           Text {
               text: monitor.realTimeEnabled
                     ? "你的设备正在被保护，一切正常。"
                     : "⚠ 实时保护已关闭，你的设备存在风险！"
               font.pixelSize: 14
               color: monitor.realTimeEnabled ? "#4CAF50" : "#F44336"
           }

           // ── Protection days banner ──────────────────────────
           Card {
               Layout.fillWidth: true; padding: 20
               implicitHeight: 140
               color: "#E8F5E9"
               RowLayout {
                   spacing: 16
                   Rectangle {
                       width: 52; height: 52; radius: 26
                       color: "#4CAF50"
                       Text {
                           anchors.centerIn: parent
                           text: "🛡"; font.pixelSize: 26
                       }
                   }
                   ColumnLayout {
                       spacing: 2
                       Text {
                           text: "已守护 " + mainWindow.engineManager.protectionDays + " 天"
                           font.pixelSize: 26; font.bold: true
                           color: "#2E7D32"
                       }
                       Text {
                           text: "Zack Defender 持续保护您的电脑安全"
                           font.pixelSize: 13
                           color: "#4CAF50"
                       }
                   }
                   Item { Layout.fillWidth: true }
                   Image {
                       source: "qrc:/assets/shield_mascot.png"
                       fillMode: Image.PreserveAspectFit
                       Layout.preferredWidth: 100
                       Layout.preferredHeight: 100
                       Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                   }
               }
           }

           // ── Status Cards ────────────────────────────────────
           RowLayout {
               spacing: 16

               Card {
                   Layout.fillWidth: true
                   Layout.preferredHeight: 130
                   padding: 20

                   ColumnLayout {
                       spacing: 12
                       RowLayout {
                           spacing: 12
                           Rectangle {
                               width: 48; height: 48; radius: 24
                               color: monitor.realTimeEnabled ? "#4CAF50" : "#F44336"
                               Rectangle {
                                   anchors.centerIn: parent
                                   width: parent.width; height: parent.height
                                   radius: parent.radius
                                   color: parent.color
                                   opacity: 0.3
                                   NumberAnimation on scale {
                                       from: 1.0; to: 1.6
                                       duration: 1500; loops: Animation.Infinite
                                       running: monitor.realTimeEnabled
                                   }
                                   NumberAnimation on opacity {
                                       from: 0.3; to: 0.0
                                       duration: 1500; loops: Animation.Infinite
                                       running: monitor.realTimeEnabled
                                   }
                               }
                               Text {
                                   anchors.centerIn: parent
                                   text: "🛡"; font.pixelSize: 24
                               }
                           }
                           ColumnLayout {
                               spacing: 2
                               Text {
                                   text: "实时保护"
                                   font.pixelSize: 18; font.bold: true
                                   color: Theme.color.onSurfaceColor
                               }
                               Text {
                                   text: monitor.realTimeEnabled ? "已开启" : "已关闭"
                                   font.pixelSize: 13; color: Theme.color.onSurfaceVariantColor
                               }
                           }
                       }
                   }
               }

               Card {
                   Layout.fillWidth: true
                   Layout.preferredHeight: 130
                   padding: 20

                   ColumnLayout {
                       spacing: 12
                       RowLayout {
                           spacing: 12
                           Rectangle {
                               width: 48; height: 48; radius: 24
                               color: mainWindow.threatHistory.threatCount > 0 ? "#F44336" : "#4CAF50"
                               Text {
                                   anchors.centerIn: parent
                                   text: mainWindow.threatHistory.threatCount > 0 ? "⚠" : "✓"
                                   font.pixelSize: 24
                               }
                           }
                           ColumnLayout {
                               spacing: 2
                               Text {
                                   text: "检测到的威胁"
                                   font.pixelSize: 18; font.bold: true
                                   color: Theme.color.onSurfaceColor
                               }
                               Text {
                                   text: mainWindow.threatHistory.threatCount + " 个"
                                   font.pixelSize: 28; font.bold: true
                                   color: mainWindow.threatHistory.threatCount > 0 ? "#F44336" : "#4CAF50"
                               }
                           }
                       }
                   }
               }
           }

           // ── Second row ──────────────────────────────────────
           RowLayout {
               spacing: 16

               Card {
                   Layout.fillWidth: true
                   Layout.preferredHeight: 100
                   padding: 20

                   ColumnLayout {
                       spacing: 8
                       Text {
                           text: "上次扫描"
                           font.pixelSize: 14; color: Theme.color.onSurfaceVariantColor
                       }
                       Text {
                           text: mainWindow.engineManager.lastScanType || "尚未扫描"
                           font.pixelSize: 18; font.bold: true
                           color: Theme.color.onSurfaceColor
                       }
                       Text {
                           visible: mainWindow.engineManager.lastScanTime !== ""
                           text: mainWindow.engineManager.lastScanTime
                           font.pixelSize: 12; color: Theme.color.onSurfaceVariantColor
                       }
                   }
               }

               Card {
                   Layout.fillWidth: true
                   Layout.preferredHeight: 100
                   padding: 20

                   ColumnLayout {
                       spacing: 8
                       Text {
                           text: "签名版本"
                           font.pixelSize: 14; color: Theme.color.onSurfaceVariantColor
                       }
                       Text {
                           text: monitor.signatureVersion || "获取中..."
                           font.pixelSize: 18; font.bold: true
                           color: Theme.color.onSurfaceColor
                       }
                       Text {
                           text: "更新: " + (monitor.lastUpdate || "-")
                           font.pixelSize: 12; color: Theme.color.onSurfaceVariantColor
                       }
                   }
               }
           }

           // ── Quick Actions ────────────────────────────────────
           Text {
               text: "快速操作"
               font.pixelSize: 20; font.bold: true
               color: Theme.color.onSurfaceColor
               Layout.topMargin: 4
           }

           RowLayout {
               spacing: 12
               Button {
                   text: "⚡ 快速扫描"
                   Layout.preferredWidth: 170; Layout.preferredHeight: 48
                   onClicked: mainWindow.navigateToScan(1, "")
               }
               Button {
                   text: "🔎 全面扫描"
                   Layout.preferredWidth: 170; Layout.preferredHeight: 48
                   onClicked: mainWindow.navigateToScan(2, "")
               }
               Button {
                   text: "📁 自定义扫描"
                   Layout.preferredWidth: 170; Layout.preferredHeight: 48
                   onClicked: mainWindow.startCustomScan()
               }
           }

           // ── Engine Info ─────────────────────────────────────
           Card {
               Layout.fillWidth: true
               implicitHeight: 140
               padding: 20

               ColumnLayout {
                   spacing: 12
                   Text {
                       text: "引擎信息"
                       font.pixelSize: 16; font.bold: true
                       color: Theme.color.onSurfaceColor
                   }
                   GridLayout {
                       columns: 2; rowSpacing: 8; columnSpacing: 32
                       Text { text: "活跃引擎:"; color: Theme.color.onSurfaceVariantColor; font.pixelSize: 14 }
                       Text {
                           text: mainWindow.engineManager.activeEngineCount + " 个 ("
                               + mainWindow.engineManager.engineNames.join(", ") + ")"
                           color: Theme.color.onSurfaceColor; font.pixelSize: 14
                           Layout.maximumWidth: 350; elide: Text.ElideRight
                       }
                       Text { text: "引擎版本:"; color: Theme.color.onSurfaceVariantColor; font.pixelSize: 14 }
                       Text { text: monitor.engineVersion || "-"; color: Theme.color.onSurfaceColor; font.pixelSize: 14 }
                       Text { text: "UI 框架:"; color: Theme.color.onSurfaceVariantColor; font.pixelSize: 14 }
                       Text { text: "Qt 6.11 + MD3"; color: Theme.color.onSurfaceColor; font.pixelSize: 14 }
                   }
               }
           }
       }
   }
}

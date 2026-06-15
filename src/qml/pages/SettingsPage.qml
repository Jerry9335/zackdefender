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
    ThreatHistory { id: history; Component.onCompleted: refresh() }

    property bool autoRefresh: false

    Flickable {
        anchors.fill: parent
        contentHeight: contentLayout.implicitHeight
        clip: true

        ColumnLayout {
            id: contentLayout
            width: parent.width
            spacing: 24

            RowLayout {
                Text {
                    text: "设置 & 历史"
                    font.pixelSize: 28; font.bold: true
                    color: Theme.color.onSurfaceColor
                }
                Item { Layout.fillWidth: true }
                Button {
                    text: "回到仪表盘"
                    onClicked: mainWindow.goToDashboard()
                }
            }

            // ── Protection status ────────────────────────────────
            Text {
                text: "保护状态"
                font.pixelSize: 20; font.bold: true
                color: Theme.color.onSurfaceColor
            }

            Card {
                Layout.fillWidth: true; padding: 20

                GridLayout {
                    columns: 2; rowSpacing: 12; columnSpacing: 24

                    Text { text: "实时保护:"; color: Theme.color.onSurfaceVariantColor; font.pixelSize: 14 }
                    RowLayout {
                        spacing: 8
                        Rectangle {
                            width: 12; height: 12; radius: 6
                            color: monitor.realTimeEnabled ? "#4CAF50" : "#F44336"
                        }
                        Text {
                            text: monitor.realTimeEnabled ? "已开启" : "已关闭"
                            font.pixelSize: 14; color: Theme.color.onSurfaceColor
                        }
                    }

                    Text { text: "防病毒:"; color: Theme.color.onSurfaceVariantColor; font.pixelSize: 14 }
                    Text {
                        text: monitor.antivirusEnabled ? "已启用" : "已禁用"
                        font.pixelSize: 14; color: Theme.color.onSurfaceColor
                    }

                    Text { text: "引擎版本:"; color: Theme.color.onSurfaceVariantColor; font.pixelSize: 14 }
                    Text {
                        text: monitor.engineVersion || "-"
                        font.pixelSize: 14; color: Theme.color.onSurfaceColor
                    }

                    Text { text: "签名版本:"; color: Theme.color.onSurfaceVariantColor; font.pixelSize: 14 }
                    Text {
                        text: monitor.signatureVersion || "-"
                        font.pixelSize: 14; color: Theme.color.onSurfaceColor
                    }

                    Text { text: "最后更新:"; color: Theme.color.onSurfaceVariantColor; font.pixelSize: 14 }
                    Text {
                        text: monitor.lastUpdate || "-"
                        font.pixelSize: 14; color: Theme.color.onSurfaceColor
                    }
                }
            }

            // ── Auto-refresh ─────────────────────────────────────
            Card {
                Layout.fillWidth: true; padding: 20

                ColumnLayout {
                    spacing: 12

                    RowLayout {
                        spacing: 16
                        ColumnLayout {
                            spacing: 4
                            Text {
                                text: "自动刷新"
                                font.pixelSize: 16; font.bold: true
                                color: Theme.color.onSurfaceColor
                            }
                            Text {
                                text: "每 15 秒自动刷新保护状态"
                                font.pixelSize: 13; color: Theme.color.onSurfaceVariantColor
                            }
                        }
                        Item { Layout.fillWidth: true }
                        Switch {
                            id: autoRefreshSwitch
                            checked: autoRefresh
                            onCheckedChanged: {
                                autoRefresh = checked
                                if (autoRefresh) {
                                    monitor.startAutoRefresh(15000)
                                } else {
                                    monitor.stopAutoRefresh()
                                }
                            }
                        }
                    }

                    RowLayout {
                        spacing: 10
                        visible: autoRefresh
                        Button {
                            text: "🔄 立即刷新"
                            onClicked: { monitor.refresh(); history.refresh() }
                        }
                    }
                }
            }

            // ── Theme ──────────────────────────────────────────────
            Text {
                text: "外观主题"
                font.pixelSize: 20; font.bold: true
                color: Theme.color.onSurfaceColor
            }

            Card {
                Layout.fillWidth: true; padding: 20

                ColumnLayout {
                    spacing: 16
                    Layout.fillWidth: true

                    Text {
                        text: "选择主题模式"
                        font.pixelSize: 16; font.bold: true
                        color: Theme.color.onSurfaceColor
                    }

                    Text {
                        text: mainWindow.themeManager.themeMode === 0
                              ? (mainWindow.themeManager.isSystemDark
                                 ? "🌙 当前跟随系统：深色模式"
                                 : "☀️ 当前跟随系统：浅色模式")
                              : ""
                        font.pixelSize: 13
                        color: Theme.color.onSurfaceVariantColor
                        visible: mainWindow.themeManager.themeMode === 0
                    }

                    RowLayout {
                        spacing: 10
                        Button {
                            text: "🌓 跟随系统"
                            implicitHeight: 40
                            type: mainWindow.themeManager.themeMode === 0 ? "filled" : "outlined"
                            onClicked: mainWindow.themeManager.themeMode = 0
                        }
                        Button {
                            text: "☀️ 浅色"
                            implicitHeight: 40
                            type: mainWindow.themeManager.themeMode === 1 ? "filled" : "outlined"
                            onClicked: mainWindow.themeManager.themeMode = 1
                        }
                        Button {
                            text: "🌙 深色"
                            implicitHeight: 40
                            type: mainWindow.themeManager.themeMode === 2 ? "filled" : "outlined"
                            onClicked: mainWindow.themeManager.themeMode = 2
                        }
                    }
                }
            }

            // ── Threat History ───────────────────────────────────
            Text {
                text: "威胁历史"
                font.pixelSize: 20; font.bold: true
                color: Theme.color.onSurfaceColor
            }

            Card {
                Layout.fillWidth: true
                Layout.preferredHeight: 300
                padding: 16

                ColumnLayout {
                    anchors.fill: parent; spacing: 12

                    RowLayout {
                        Text {
                            text: "共 " + history.threatCount + " 条记录"
                            font.pixelSize: 14; color: Theme.color.onSurfaceVariantColor
                        }
                        Item { Layout.fillWidth: true }
                        Button {
                            text: "🔄 刷新"
                            onClicked: history.refresh()
                        }
                        Button {
                            text: "🗑 清除"
                            enabled: history.threatCount > 0
                            onClicked: history.clearHistory()
                        }
                    }

                    ScrollView {
                        Layout.fillWidth: true; Layout.fillHeight: true; clip: true

                        ListView {
                            id: threatList
                            model: history.threats

                            delegate: Card {
                                width: threatList.width - 8; padding: 12

                                ColumnLayout {
                                    spacing: 4
                                    anchors.fill: parent

                                    RowLayout {
                                        spacing: 8
                                        Text {
                                            text: "⚠"
                                            font.pixelSize: 16
                                            color: modelData.severity <= 3 ? "#F44336" : "#FF9800"
                                        }
                                        Text {
                                            text: modelData.threatName || "未知威胁"
                                            font.pixelSize: 14; font.bold: true
                                            color: Theme.color.onSurfaceColor
                                            Layout.fillWidth: true
                                            elide: Text.ElideRight
                                        }
                                    }

                                    Text {
                                        text: "路径: " + (modelData.filePath || "-")
                                        font.pixelSize: 12; color: Theme.color.onSurfaceVariantColor
                                        Layout.fillWidth: true; elide: Text.ElideMiddle
                                    }

                                    RowLayout {
                                        spacing: 16
                                        Text {
                                            text: "操作: " + (modelData.action || "-")
                                            font.pixelSize: 12; color: Theme.color.onSurfaceVariantColor
                                        }
                                        Text {
                                            text: "严重性: " + (modelData.severity || "-")
                                            font.pixelSize: 12; color: Theme.color.onSurfaceVariantColor
                                        }
                                        Text {
                                            text: "时间: " + (modelData.detectedAt
                                                  ? new Date(modelData.detectedAt).toLocaleString()
                                                  : "-")
                                            font.pixelSize: 12; color: Theme.color.onSurfaceVariantColor
                                        }
                                    }
                                }
                            }

                            Text {
                                anchors.centerIn: parent
                                text: "安全无威胁记录 🎉"
                                font.pixelSize: 16; color: Theme.color.onSurfaceVariantColor
                                visible: threatList.count === 0
                            }
                        }
                    }
                }
            }

            // ── About ────────────────────────────────────────────
            Text {
                text: "关于"
                font.pixelSize: 20; font.bold: true
                color: Theme.color.onSurfaceColor
            }

            Card {
                Layout.fillWidth: true; padding: 24
                implicitHeight: 280

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 16
                    GridLayout {
                        Layout.fillWidth: true
                        columns: 2; rowSpacing: 10; columnSpacing: 24
                        Text { text: "版本:"; color: Theme.color.onSurfaceVariantColor; font.pixelSize: 14 }
                        Text { text: "1.2.0"; color: Theme.color.onSurfaceColor; font.pixelSize: 14 }
                        Text { text: "扫描引擎:"; color: Theme.color.onSurfaceVariantColor; font.pixelSize: 14 }
                        Text { text: "Windows Defender (MpCmdRun)"; color: Theme.color.onSurfaceColor; font.pixelSize: 14 }
                        Text { text: "UI 框架:"; color: Theme.color.onSurfaceVariantColor; font.pixelSize: 14 }
                        Text { text: "Qt 6.11 + Material Design 3"; color: Theme.color.onSurfaceColor; font.pixelSize: 14 }
                    }

                    Rectangle { Layout.fillWidth: true; height: 1; color: Theme.color.outlineVariant }

                    Text {
                        text: "Zack Defender 是基于 Windows Defender 引擎的安全管理工具，"
                            + "提供病毒扫描、实时保护监控、威胁历史和隔离区管理。"
                        font.pixelSize: 14; color: Theme.color.onSurfaceVariantColor
                        Layout.maximumWidth: 600; wrapMode: Text.WordWrap
                    }

                    Button {
                        text: "📋 更新日志"
                        Layout.fillWidth: true
                        Layout.preferredHeight: 40
                        onClicked: mainWindow.showChangelog()
                    }
                }
            }

            // Bottom spacer so last card isn't cut off
            Item { Layout.preferredHeight: 24 }
        }
    }

    // Clean up auto-refresh when leaving page
    Component.onDestruction: {
        if (autoRefresh) monitor.stopAutoRefresh()
    }
}

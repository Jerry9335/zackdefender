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

    ColumnLayout {
        anchors.fill: parent
        spacing: 20

        // ── Header ──────────────────────────────────────────────
        RowLayout {
            Text {
                text: "实时保护"
                font.pixelSize: 28; font.bold: true
                color: Theme.color.onSurfaceColor
            }
            Item { Layout.fillWidth: true }
            Rectangle {
                radius: 8
                color: monitor.realTimeEnabled ? "#4CAF50" : "#F44336"
                implicitWidth: protChipText.width + 24
                implicitHeight: 28
                Text {
                    id: protChipText
                    anchors.centerIn: parent
                    text: monitor.realTimeEnabled ? "已保护" : "有风险"
                    font.pixelSize: 12; color: "white"
                }
            }
            Button {
                text: "回到仪表盘"
                onClicked: mainWindow.goToDashboard()
            }
        }

        // ── Main status card ────────────────────────────────────
        Card {
            Layout.fillWidth: true; padding: 28

            RowLayout {
                spacing: 24
                Rectangle {
                    width: 80; height: 80; radius: 40
                    color: monitor.realTimeEnabled ? "#4CAF50" : "#F44336"
                    Rectangle {
                        anchors.centerIn: parent
                        width: parent.width; height: parent.height
                        radius: parent.radius; color: parent.color
                        opacity: 0.25
                        NumberAnimation on scale {
                            from: 1.0; to: 1.5; duration: 2000
                            loops: Animation.Infinite
                            running: monitor.realTimeEnabled
                        }
                        NumberAnimation on opacity {
                            from: 0.25; to: 0.0; duration: 2000
                            loops: Animation.Infinite
                            running: monitor.realTimeEnabled
                        }
                    }
                    Text {
                        anchors.centerIn: parent
                        text: "🛡"; font.pixelSize: 36
                    }
                }
                ColumnLayout {
                    spacing: 8
                    Text {
                        text: monitor.realTimeEnabled ? "实时保护已开启" : "实时保护已关闭"
                        font.pixelSize: 24; font.bold: true
                        color: Theme.color.onSurfaceColor
                    }
                    Text {
                        text: monitor.realTimeEnabled
                              ? "Zack Defender 正在主动监控系统，阻止恶意软件。"
                              : "⚠ 你的设备存在风险！建议立即开启实时保护。"
                        font.pixelSize: 14; color: Theme.color.onSurfaceVariantColor
                        Layout.maximumWidth: 500; wrapMode: Text.WordWrap
                    }
                }
            }
        }

        // ── Protection components ───────────────────────────────
        Text {
            text: "保护组件"
            font.pixelSize: 20; font.bold: true
            color: Theme.color.onSurfaceColor
            Layout.topMargin: 4
        }

        GridLayout {
            columns: 2; rowSpacing: 14; columnSpacing: 14

            Repeater {
                model: [
                    { name: "病毒和威胁防护", icon: "🦠", desc: "扫描和移除恶意软件",
                      enabled: monitor.antivirusEnabled },
                    { name: "实时行为监控", icon: "👁️", desc: "监控可疑程序行为",
                      enabled: monitor.realTimeEnabled },
                    { name: "云提供的保护", icon: "☁️", desc: "Microsoft 云端安全情报",
                      enabled: true },
                    { name: "自动样本提交", icon: "📤", desc: "提交可疑文件以供分析",
                      enabled: true }
                ]
                Card {
                    Layout.fillWidth: true; Layout.preferredHeight: 90
                    padding: 18

                    RowLayout {
                        spacing: 14
                        Rectangle {
                            width: 44; height: 44; radius: 22
                            color: modelData.enabled ? "#4CAF50" : "#F44336"
                            Text {
                                anchors.centerIn: parent
                                text: modelData.icon; font.pixelSize: 22
                            }
                        }
                        ColumnLayout {
                            spacing: 4
                            Text {
                                text: modelData.name
                                font.pixelSize: 15; font.bold: true
                                color: Theme.color.onSurfaceColor
                            }
                            Text {
                                text: modelData.desc
                                font.pixelSize: 12; color: Theme.color.onSurfaceVariantColor
                            }
                        }
                        Item { Layout.fillWidth: true }
                        Rectangle {
                            radius: 6
                            color: modelData.enabled ? "#4CAF50" : "#F44336"
                            implicitWidth: enText.width + 20; implicitHeight: 24
                            Text {
                                id: enText
                                anchors.centerIn: parent
                                text: modelData.enabled ? "已启用" : "已禁用"
                                font.pixelSize: 11; color: "white"
                            }
                        }
                    }
                }
            }
        }

        // ── Bottom actions ──────────────────────────────────────
        Card {
            Layout.fillWidth: true; padding: 18

            RowLayout {
                spacing: 12
                Button {
                    text: "🔄 刷新保护状态"; Layout.preferredHeight: 42
                    onClicked: monitor.refresh()
                }
                Item { Layout.fillWidth: true }
                ColumnLayout {
                    spacing: 2
                    Text {
                        text: "引擎版本: " + (monitor.engineVersion || "-")
                        font.pixelSize: 13; color: Theme.color.onSurfaceVariantColor
                    }
                    Text {
                        text: "签名: " + (monitor.signatureVersion || "-")
                        font.pixelSize: 13; color: Theme.color.onSurfaceVariantColor
                    }
                }
            }
        }
    }
}

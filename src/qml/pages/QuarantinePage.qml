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

    // ── Use global instance from Main.qml ────────────────────
    property var quarantineManager: mainWindow.quarantine

    ColumnLayout {
        anchors.fill: parent
        spacing: 24

        RowLayout {
            Text {
                text: "隔离区"
                font.pixelSize: 28; font.bold: true
                color: Theme.color.onSurfaceColor
            }
            Item { Layout.fillWidth: true }
            Button {
                text: "回到仪表盘"
                onClicked: mainWindow.goToDashboard()
            }
        }

        // ── Summary card ─────────────────────────────────────────
        Card {
            Layout.fillWidth: true; padding: 20

            RowLayout {
                spacing: 16
                Text { text: "📦"; font.pixelSize: 40 }
                ColumnLayout {
                    spacing: 4
                    Text {
                        text: quarantineManager.itemCount + " 个文件在隔离区"
                        font.pixelSize: 20; font.bold: true
                        color: Theme.color.onSurfaceColor
                    }
                    Text {
                        text: "包含 Windows Defender 隔离记录和本地隔离文件。"
                        font.pixelSize: 13; color: Theme.color.onSurfaceVariantColor
                        Layout.maximumWidth: 500; wrapMode: Text.WordWrap
                    }
                }
                Item { Layout.fillWidth: true }
                Button {
                    text: "🗑 清空本地隔离区"
                    enabled: quarantineManager.itemCount > 0
                    onClicked: quarantineManager.emptyQuarantine()
                }
            }
        }

        // ── Items list ───────────────────────────────────────────
        Card {
            Layout.fillWidth: true; Layout.fillHeight: true
            padding: 16

            ColumnLayout {
                anchors.fill: parent; spacing: 12

                RowLayout {
                    Text {
                        text: "隔离文件列表"
                        font.pixelSize: 16; font.bold: true
                        color: Theme.color.onSurfaceColor
                    }
                    Item { Layout.fillWidth: true }
                    Button {
                        text: "🔄 刷新"
                        onClicked: quarantineManager.refresh()
                    }
                }

                ScrollView {
                    Layout.fillWidth: true; Layout.fillHeight: true; clip: true

                    ListView {
                        id: fileList
                        model: quarantineManager.items

                        delegate: Card {
                            width: fileList.width - 8; padding: 12

                            ColumnLayout {
                                spacing: 6
                                anchors.fill: parent

                                RowLayout {
                                    spacing: 10
                                    Text {
                                        text: modelData.source === "defender" ? "🛡️" : "📦"
                                        font.pixelSize: 18
                                    }
                                    Text {
                                        text: modelData.threatName || "未知威胁"
                                        font.pixelSize: 15; font.bold: true
                                        color: Theme.color.onSurfaceColor
                                        Layout.fillWidth: true
                                        elide: Text.ElideRight
                                    }
                                    Rectangle {
                                        radius: 4
                                        color: modelData.source === "defender"
                                               ? "#1976D2" : "#FF8F00"
                                        implicitWidth: tagText.width + 16
                                        implicitHeight: 22
                                        Text {
                                            id: tagText
                                            anchors.centerIn: parent
                                            text: modelData.source === "defender"
                                                  ? "Defender" : "本地"
                                            font.pixelSize: 11; color: "white"
                                        }
                                    }
                                }

                                Text {
                                    text: "路径: " + (modelData.filePath || "-")
                                    font.pixelSize: 12; color: Theme.color.onSurfaceVariantColor
                                    Layout.fillWidth: true
                                    elide: Text.ElideMiddle
                                }

                                RowLayout {
                                    spacing: 16
                                    Text {
                                        text: "操作: " + (modelData.action || "-")
                                        font.pixelSize: 12; color: Theme.color.onSurfaceVariantColor
                                    }
                                    Text {
                                        text: "时间: " + (modelData.detectionTime
                                              ? new Date(modelData.detectionTime).toLocaleString()
                                              : "-")
                                        font.pixelSize: 12; color: Theme.color.onSurfaceVariantColor
                                    }
                                }

                                // Only show actions for local items
                                RowLayout {
                                    spacing: 8
                                    visible: modelData.source === "local"
                                    Button {
                                        text: "↩ 恢复"
                                        implicitHeight: 28
                                        onClicked: quarantineManager.restoreFile(modelData.filePath)
                                    }
                                    Button {
                                        text: "🗑 删除"
                                        implicitHeight: 28
                                        onClicked: quarantineManager.deleteFile(modelData.filePath)
                                    }
                                }
                            }
                        }

                        Text {
                            anchors.centerIn: parent
                            text: "隔离区为空 ✨"
                            font.pixelSize: 16; color: Theme.color.onSurfaceVariantColor
                            visible: fileList.count === 0
                        }
                    }
                }
            }
        }
    }
}

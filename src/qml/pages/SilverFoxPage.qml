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

    SilverFoxEngine { id: silverFox }

    // ── Animated background — runs during scan/cleanup ──────
    IndexBackground {
        anchors.fill: parent
        running: silverFox.scanning
        z: -1
    }

    Flickable {
        anchors.fill: parent
        contentHeight: contentLayout.implicitHeight + 40
        clip: true

        ColumnLayout {
            id: contentLayout
            width: parent.width
            spacing: 20

            // Header
            RowLayout {
                Text {
                    text: "银狐临时解救"
                    font.pixelSize: 28; font.bold: true
                    color: Theme.color.onSurfaceColor
                }
                Rectangle {
                    radius: 6; color: "#FFF3E0"
                    implicitWidth: betaText.implicitWidth + 16; implicitHeight: 24
                    Text {
                        id: betaText
                        anchors.centerIn: parent
                        text: "Beta"; font.pixelSize: 11; color: "#E65100"
                    }
                }
                Item { Layout.fillWidth: true }
                Rectangle {
                    visible: !silverFox.isAdmin
                    radius: 6; color: "#FFF3E0"
                    implicitWidth: adminWarnText.implicitWidth + 16
                    implicitHeight: 24
                    Text {
                        id: adminWarnText
                        anchors.centerIn: parent
                        text: "⚠ 非管理员模式"
                        font.pixelSize: 11
                        color: "#E65100"
                    }
                }
                Button {
                    text: "回到仪表盘"
                    onClicked: mainWindow.goToDashboard()
                }
            }

            // ── Warning Banner ──────────────────────────────────
            Card {
                Layout.fillWidth: true; padding: 16
                implicitHeight: 110
                color: "#FFF3E0"
                RowLayout {
                    spacing: 12
                    Text {
                        text: "⚠"
                        font.pixelSize: 24
                    }
                    Text {
                        text: "此页面用于检测和清理银狐木马并释放安全程序，使其正常工作。\n一般情况下，不要没事使用，除非出现特殊情况。\n清理操作需要管理员权限，执行后务必重启计算机！"
                        font.pixelSize: 13; color: "#E65100"
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                }
            }

            // ── Admin Permission Check ──────────────────────────
            Card {
                Layout.fillWidth: true; padding: 16
                implicitHeight: 80
                color: silverFox.isAdmin ? "#E8F5E9" : "#FFEBEE"

                RowLayout {
                    spacing: 12
                    Text {
                        text: silverFox.isAdmin ? "✅" : "⚠"
                        font.pixelSize: 22
                    }
                    ColumnLayout {
                        spacing: 2
                        Text {
                            text: silverFox.isAdmin ? "管理员权限已获取" : "需要管理员权限"
                            font.pixelSize: 15; font.bold: true
                            color: silverFox.isAdmin ? "#2E7D32" : "#C62828"
                        }
                        Text {
                            text: silverFox.isAdmin
                                ? "可以使用银狐病毒检测功能"
                                : "银狐检测和清理需要管理员权限才能运行"
                            font.pixelSize: 12
                            color: silverFox.isAdmin ? "#4CAF50" : "#E53935"
                        }
                    }
                    Item { Layout.fillWidth: true }
                    Button {
                        visible: !silverFox.isAdmin
                        text: "🔐 以管理员身份重启"
                        implicitHeight: 38
                        onClicked: silverFox.restartAsAdmin()
                    }
                }
            }

            // ── Detection Card ──────────────────────────────────
            Card {
                Layout.fillWidth: true; padding: 20

                ColumnLayout {
                    spacing: 14
                    Text {
                        text: "🔍 银狐病毒检测"
                        font.pixelSize: 18; font.bold: true
                        color: Theme.color.onSurfaceColor
                    }
                    Text {
                        text: "检测项目：隐藏/系统属性进程 · 注册表 Run 启动项 · 可疑服务 · WDAC 策略 · DLL 劫持"
                        font.pixelSize: 12; color: Theme.color.onSurfaceVariantColor
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }

                    RowLayout {
                        spacing: 10
                        Button {
                            text: silverFox.scanning ? "⏳ 检测中..."
                                : (!silverFox.isAdmin ? "🔒 需要管理员权限" : "🔍 开始检测")
                            enabled: !silverFox.scanning && silverFox.isAdmin
                            Layout.preferredHeight: 42
                            onClicked: silverFox.startDetection()
                        }
                        Button {
                            text: "重置结果"
                            enabled: !silverFox.scanning && silverFox.threatsFound > 0
                            onClicked: silverFox.reset()
                        }
                        Item { Layout.fillWidth: true }
                        Image {
                            source: "qrc:/assets/yinhu.png"
                            fillMode: Image.PreserveAspectFit
                            Layout.preferredWidth: 100
                            Layout.preferredHeight: 100
                            Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                        }
                    }

                    // Progress
                    ColumnLayout {
                        visible: silverFox.scanning
                        spacing: 10
                        Layout.fillWidth: true

                        RowLayout {
                            spacing: 12
                            LoadingIndicator {
                                Layout.preferredWidth: 32; Layout.preferredHeight: 32
                                running: true; size: 32
                            }
                            Text {
                                text: silverFox.progress + "%"
                                font.pixelSize: 18; font.bold: true
                                color: Theme.color.onSurfaceColor
                            }
                        }
                        LinearProgress {
                            Layout.fillWidth: true
                            wavy: true
                            indeterminate: true
                            value: silverFox.progress / 100.0
                        }
                    }
                }
            }

            // ── Results Card ────────────────────────────────────
            Card {
                Layout.fillWidth: true; padding: 16
                visible: silverFox.threatsFound > 0 || (!silverFox.scanning && silverFox.results.length > 0)

                ColumnLayout {
                    spacing: 10
                    Text {
                        text: silverFox.threatsFound > 0
                            ? "⚠ 发现 " + silverFox.threatsFound + " 个可疑项"
                            : "✅ 未发现银狐病毒特征"
                        font.pixelSize: 16; font.bold: true
                        color: silverFox.threatsFound > 0 ? "#C62828" : "#2E7D32"
                    }

                    Column {
                        spacing: 6
                        Layout.fillWidth: true
                        visible: silverFox.threatsFound > 0
                        Repeater {
                            model: silverFox.results
                            delegate: Rectangle {
                                width: parent.width
                                height: 48
                                radius: 6
                                color: modelData.severity >= 4 ? "#FFEBEE" : "#FFF8E1"
                                border { width: 1; color: modelData.severity >= 4 ? "#FFCDD2" : "#FFE0B2" }

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.margins: 10
                                    spacing: 10
                                    Text {
                                        text: "[" + (modelData.type || "?") + "]"
                                        font.pixelSize: 11; font.bold: true
                                        color: Theme.color.onSurfaceVariantColor
                                    }
                                    Text {
                                        text: modelData.path || ""
                                        font.pixelSize: 12
                                        color: Theme.color.onSurfaceColor
                                        elide: Text.ElideMiddle
                                        Layout.fillWidth: true
                                    }
                                    Text {
                                        text: modelData.desc || ""
                                        font.pixelSize: 11
                                        color: Theme.color.onSurfaceVariantColor
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // ── Cleanup Card ────────────────────────────────────
            Card {
                Layout.fillWidth: true; padding: 20
                visible: silverFox.threatsFound > 0

                ColumnLayout {
                    spacing: 14
                    Text {
                        text: "🛡 紧急清理"
                        font.pixelSize: 18; font.bold: true
                        color: Theme.color.onSurfaceColor
                    }
                    Text {
                        text: "清理操作：移除 WDAC 策略 · 禁用任务计划 · 劫持+终止可疑进程 · 禁用可疑服务 · 清理恶意 DLL\n⚠ 需要管理员权限！执行后必须重启！"
                        font.pixelSize: 12; color: Theme.color.onSurfaceVariantColor
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }

                    Button {
                        text: silverFox.scanning ? "⏳ 清理中..."
                            : (!silverFox.isAdmin ? "🔒 需要管理员权限" : "🛡 开始紧急清理")
                        enabled: !silverFox.scanning && silverFox.isAdmin
                        Layout.preferredHeight: 42
                        onClicked: {
                            cleanupConfirmDialog.open()
                        }
                    }
                }
            }

            // ── Log ─────────────────────────────────────────────
            Card {
                Layout.fillWidth: true; Layout.fillHeight: true
                implicitHeight: 200; padding: 12

                ColumnLayout {
                    anchors.fill: parent; spacing: 8
                    Text {
                        text: "操作日志"
                        font.pixelSize: 14; font.bold: true
                        color: Theme.color.onSurfaceColor
                    }
                    ScrollView {
                        Layout.fillWidth: true; Layout.fillHeight: true
                        clip: true
                        TextEdit {
                            id: logArea
                            readOnly: true
                            font.pixelSize: 12
                            font.family: "Consolas, monospace"
                            color: Theme.color.onSurfaceVariantColor
                            wrapMode: TextEdit.Wrap
                            textFormat: TextEdit.PlainText
                            width: parent.width
                        }
                    }
                }
            }
        }
    }

    // ── Cleanup confirmation dialog ─────────────────────────
    Dialog {
        id: cleanupConfirmDialog
        title: "⚠ 确认执行紧急清理？"
        acceptText: "确认清理"
        showRejectButton: true
        rejectText: "取消"
        onAccepted: silverFox.runCleanupScript()

        content: ColumnLayout {
            spacing: 12
            Text {
                text: "此操作将：\n\n" +
                      "• 移除 WDAC 策略（解锁杀毒软件）\n" +
                      "• 劫持并终止所有可疑进程\n" +
                      "• 禁用所有可疑服务\n" +
                      "• 禁用根目录任务计划\n" +
                      "• 清理恶意 DLL 文件\n\n" +
                      "⚠ 执行后必须立即重启计算机！"
                font.pixelSize: 13
                color: Theme.color.onSurfaceColor
                wrapMode: Text.WordWrap
            }
        }
    }

    // ── Log connection ──────────────────────────────────────
    Connections {
        target: silverFox
        function onLogMessage(msg) {
            logArea.text += msg + "\n"
            // Auto-scroll
            logArea.cursorPosition = logArea.text.length
        }
        function onDetectionFinished(count) {
            logArea.text += "\n=== 检测完成，发现 " + count + " 个可疑项 ===\n"
            logArea.cursorPosition = logArea.text.length
        }
    }
}

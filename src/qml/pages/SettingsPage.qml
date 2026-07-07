import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import Qt.labs.settings
import md3.Core
import zack.defender

Item {
   id: root

   // ── Entrance animation ──────────────────────────────────
   opacity: 0; y: 12
   Component.onCompleted: {
       opacity = 1; y = 0
       // Restore auto-refresh state
       autoRefresh = refreshSettings.enabled
       if (autoRefresh) monitor.startAutoRefresh(15000)
   }
   Behavior on opacity { NumberAnimation { duration: 200; easing.type: Easing.OutCubic } }
   Behavior on y { NumberAnimation { duration: 250; easing.type: Easing.OutCubic } }

   Settings { id: refreshSettings; category: "autoRefresh"; property bool enabled: false }

   ProtectionMonitor { id: monitor; Component.onCompleted: refresh() }

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
               implicitHeight: 120  // single switch row

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
                               if (autoRefresh === checked) return
                               autoRefresh = checked
                               refreshSettings.enabled = checked
                               if (checked) monitor.startAutoRefresh(15000)
                               else monitor.stopAutoRefresh()
                           }
                       }
                   }

                   RowLayout {
                       spacing: 10
                       visible: autoRefresh
                       Button {
                           text: "🔄 立即刷新"
                           onClicked: { monitor.refresh(); mainWindow.threatHistory.refresh() }
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
               implicitHeight: 340

               ColumnLayout {
                   spacing: 14
                   Layout.fillWidth: true

                   // ── Theme mode ──────────────────────────────
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
                           text: "🌓 跟随系统"; implicitHeight: 40
                           type: mainWindow.themeManager.themeMode === 0 ? "filled" : "outlined"
                           onClicked: mainWindow.themeManager.themeMode = 0
                       }
                       Button {
                           text: "☀️ 浅色"; implicitHeight: 40
                           type: mainWindow.themeManager.themeMode === 1 ? "filled" : "outlined"
                           onClicked: mainWindow.themeManager.themeMode = 1
                       }
                       Button {
                           text: "🌙 深色"; implicitHeight: 40
                           type: mainWindow.themeManager.themeMode === 2 ? "filled" : "outlined"
                           onClicked: mainWindow.themeManager.themeMode = 2
                       }
                   }

                   Rectangle { Layout.fillWidth: true; height: 1; color: Theme.color.outlineVariant }

                   // ── Accent color ────────────────────────────
                   Text {
                       text: "主题色"
                       font.pixelSize: 16; font.bold: true
                       color: Theme.color.onSurfaceColor
                   }

                   RowLayout {
                       spacing: 10
                       Button {
                           text: "🖼 从壁纸提取"
                           implicitHeight: 36
                           onClicked: {
                               var c = mainWindow.themeManager.extractWallpaperColor();
                               StyleManager.seedColor = c;
                               mainWindow.themeManager.applySeedColor(c);
                           }
                       }
                       Button {
                           text: "🔄 恢复默认"
                           implicitHeight: 36
                           onClicked: {
                               StyleManager.seedColor = "#6750A4";  // MD3 default purple
                               mainWindow.themeManager.customSeedColor = Qt.rgba(0,0,0,0);
                           }
                       }
                   }

                   // ── Preset color swatches ──────────────────
                   RowLayout {
                       id: presetRow
                       spacing: 8
                       property var presets: [
                           "#6750A4", "#1976D2", "#009688", "#E64A19",
                           "#C2185B", "#7B1FA2", "#388E3C", "#F57C00"
                       ]
                       Repeater {
                           model: presetRow.presets
                           Rectangle {
                               width: 28; height: 28; radius: 14
                               color: modelData
                               border { width: 2; color: Theme.color.outline }
                               MouseArea {
                                   anchors.fill: parent
                                   cursorShape: Qt.PointingHandCursor
                                   onClicked: {
                                       StyleManager.seedColor = modelData;
                                       mainWindow.themeManager.applySeedColor(modelData);
                                   }
                               }
                           }
                       }
                       Item { Layout.fillWidth: true }
                       // Custom color button
                       Rectangle {
                           width: 28; height: 28; radius: 14
                           color: "transparent"
                           border { width: 2; color: Theme.color.primary }
                           Text {
                               anchors.centerIn: parent
                               text: "+"; font.pixelSize: 16; color: Theme.color.primary
                           }
                           MouseArea {
                               anchors.fill: parent
                               cursorShape: Qt.PointingHandCursor
                               onClicked: colorDialog.open()
                           }
                       }
                   }
               }
           }

           // ── Engine Management ────────────────────────────────────
            Text {
                text: "扫描引擎"
                font.pixelSize: 20; font.bold: true
                color: Theme.color.onSurfaceColor
            }

            Card {
                id: engineCard
                Layout.fillWidth: true; padding: 20
                implicitHeight: 320  // 4 engines + spacing

                ColumnLayout {
                    id: engineCardContent
                    spacing: 12
                    Layout.fillWidth: true

                    Text {
                        text: "当前活跃: " + mainWindow.engineManager.activeEngineCount + " 个引擎"
                        font.pixelSize: 14; font.bold: true
                        color: Theme.color.onSurfaceColor
                    }

                    // ── Defender engine ─────────────────────────
                    RowLayout {
                        spacing: 16
                        ColumnLayout {
                            spacing: 2
                            Text {
                                text: "🛡 Windows Defender"
                                font.pixelSize: 15; font.bold: true
                                color: Theme.color.onSurfaceColor
                            }
                            Text {
                                text: "系统内置引擎，通过 MpCmdRun 调用"
                                font.pixelSize: 12; color: Theme.color.onSurfaceVariantColor
                            }
                        }
                        Item { Layout.fillWidth: true }
                        Switch {
                            checked: mainWindow.engineManager.isEngineEnabled("Windows Defender")
                            onCheckedChanged: mainWindow.engineManager.setEngineEnabled("Windows Defender", checked)
                        }
                    }

                    Rectangle { Layout.fillWidth: true; height: 1; color: Theme.color.outlineVariant }

                    // ── Hash engine ────────────────────────────
                    RowLayout {
                        spacing: 16
                        ColumnLayout {
                            spacing: 2
                            Text {
                                text: "🔑 哈希检测"
                                font.pixelSize: 15; font.bold: true
                                color: Theme.color.onSurfaceColor
                            }
                            Text {
                                text: "MD5/SHA256 黑名单匹配，零依赖快速预扫描"
                                font.pixelSize: 12; color: Theme.color.onSurfaceVariantColor
                            }
                        }
                        Item { Layout.fillWidth: true }
                        Switch {
                            checked: mainWindow.engineManager.isEngineEnabled("哈希检测")
                            onCheckedChanged: mainWindow.engineManager.setEngineEnabled("哈希检测", checked)
                        }
                    }

                    Rectangle { Layout.fillWidth: true; height: 1; color: Theme.color.outlineVariant }

                    // ── ClamAV engine ──────────────────────────
                    RowLayout {
                        spacing: 16
                        ColumnLayout {
                            spacing: 2
                            Text {
                                text: "🦀 ClamAV"
                                font.pixelSize: 15; font.bold: true
                                color: Theme.color.onSurfaceColor
                            }
                            Text {
                                text: "开源反病毒引擎，支持 20+ 压缩格式，多线程扫描"
                                font.pixelSize: 12; color: Theme.color.onSurfaceVariantColor
                            }
                        }
                        Item { Layout.fillWidth: true }
                        Switch {
                            checked: mainWindow.engineManager.isEngineEnabled("ClamAV")
                            onCheckedChanged: mainWindow.engineManager.setEngineEnabled("ClamAV", checked)
                        }
                    }

                    Rectangle { Layout.fillWidth: true; height: 1; color: Theme.color.outlineVariant }

                    // ── YARA engine ────────────────────────────
                    RowLayout {
                        spacing: 16
                        ColumnLayout {
                            spacing: 2
                            Text {
                                text: "🔍 YARA"
                                font.pixelSize: 15; font.bold: true
                                color: Theme.color.onSurfaceColor
                            }
                            Text {
                                text: "规则引擎，基于模式匹配检测恶意软件指纹"
                                font.pixelSize: 12; color: Theme.color.onSurfaceVariantColor
                            }
                        }
                        Item { Layout.fillWidth: true }
                        Switch {
                            checked: mainWindow.engineManager.isEngineEnabled("YARA")
                            onCheckedChanged: mainWindow.engineManager.setEngineEnabled("YARA", checked)
                        }
                    }
                }
            }

            // ── ClamAV File Types ──────────────────────────────────
            Text {
                text: "🦀 ClamAV 扫描范围"
                font.pixelSize: 20; font.bold: true
                color: Theme.color.onSurfaceColor
            }

            Card {
                Layout.fillWidth: true; padding: 20
                implicitHeight: 290  // 5 type rows, just enough

                ColumnLayout {
                    spacing: 8
                    Layout.fillWidth: true

                    Text {
                        text: "选择 ClamAV 要扫描的文件类型（取消勾选可大幅提速）"
                        font.pixelSize: 13; color: Theme.color.onSurfaceVariantColor
                        Layout.fillWidth: true; wrapMode: Text.WordWrap
                    }

                    // ── Get ClamAV engine ──
                    property var clamav: mainWindow.engineManager.engineByName("ClamAV")

                    // ── 可执行文件 ──
                    RowLayout {
                        spacing: 12
                        Switch {
                            checked: parent.parent.clamav ? parent.parent.clamav.scanExecutables : true
                            onCheckedChanged: { if (parent.parent.clamav) parent.parent.clamav.scanExecutables = checked }
                        }
                        Text {
                            text: "💻 可执行文件"
                            font.pixelSize: 14; font.bold: true
                            color: Theme.color.onSurfaceColor
                        }
                        Text {
                            text: ".exe .dll .com .scr .sys .drv"
                            font.pixelSize: 12; color: Theme.color.onSurfaceVariantColor
                        }
                    }

                    // ── 脚本 ──
                    RowLayout {
                        spacing: 12
                        Switch {
                            checked: parent.parent.clamav ? parent.parent.clamav.scanScripts : true
                            onCheckedChanged: { if (parent.parent.clamav) parent.parent.clamav.scanScripts = checked }
                        }
                        Text {
                            text: "📜 脚本文件"
                            font.pixelSize: 14; font.bold: true
                            color: Theme.color.onSurfaceColor
                        }
                        Text {
                            text: ".bat .cmd .ps1 .vbs .js .wsf .wsh .hta"
                            font.pixelSize: 12; color: Theme.color.onSurfaceVariantColor
                        }
                    }

                    // ── 安装包 ──
                    RowLayout {
                        spacing: 12
                        Switch {
                            checked: parent.parent.clamav ? parent.parent.clamav.scanInstallers : true
                            onCheckedChanged: { if (parent.parent.clamav) parent.parent.clamav.scanInstallers = checked }
                        }
                        Text {
                            text: "📦 安装包"
                            font.pixelSize: 14; font.bold: true
                            color: Theme.color.onSurfaceColor
                        }
                        Text {
                            text: ".msi .pif .cpl .jar"
                            font.pixelSize: 12; color: Theme.color.onSurfaceVariantColor
                        }
                    }

                    // ── 压缩包 ──
                    RowLayout {
                        spacing: 12
                        Switch {
                            checked: parent.parent.clamav ? parent.parent.clamav.scanArchives : true
                            onCheckedChanged: { if (parent.parent.clamav) parent.parent.clamav.scanArchives = checked }
                        }
                        Text {
                            text: "🗜 压缩包（含内部扫描）"
                            font.pixelSize: 14; font.bold: true
                            color: Theme.color.onSurfaceColor
                        }
                        Text {
                            text: ".zip .7z .rar .tar .gz .bz2"
                            font.pixelSize: 12; color: Theme.color.onSurfaceVariantColor
                        }
                        Text {
                            text: "⚠ 扫描压缩包内部内容较慢"
                            font.pixelSize: 11; color: "#FF9800"
                            visible: parent.parent.clamav ? parent.parent.clamav.scanArchives : false
                        }
                    }

                    // ── 文档 ──
                    RowLayout {
                        spacing: 12
                        Switch {
                            checked: parent.parent.clamav ? parent.parent.clamav.scanDocuments : false
                            onCheckedChanged: { if (parent.parent.clamav) parent.parent.clamav.scanDocuments = checked }
                        }
                        Text {
                            text: "📄 文档文件"
                            font.pixelSize: 14; font.bold: true
                            color: Theme.color.onSurfaceColor
                        }
                        Text {
                            text: ".docm .xlsm .pptm .pdf .rtf"
                            font.pixelSize: 12; color: Theme.color.onSurfaceVariantColor
                        }
                    }
                }
            }

            // ── Context Menu ───────────────────────────────────────
           Text {
               text: "右键菜单"
               font.pixelSize: 20; font.bold: true
               color: Theme.color.onSurfaceColor
           }

           Card {
               Layout.fillWidth: true; padding: 20

               ColumnLayout {
                   spacing: 12

                   RowLayout {
                       spacing: 16
                       ColumnLayout {
                           spacing: 4
                           Text {
                               text: "右键扫描菜单"
                               font.pixelSize: 16; font.bold: true
                               color: Theme.color.onSurfaceColor
                           }
                           Text {
                               text: "在文件资源管理器中右键点击文件/文件夹，直接使用 Zack Defender 扫描"
                               font.pixelSize: 13; color: Theme.color.onSurfaceVariantColor
                               Layout.maximumWidth: 500
                               wrapMode: Text.WordWrap
                           }
                       }
                       Item { Layout.fillWidth: true }
                       Switch {
                           id: contextMenuSwitch
                           checked: ctxMenuManager.registered
                           onCheckedChanged: ctxMenuManager.setRegistered(checked)
                       }
                   }

                   Text {
                       text: ctxMenuManager.registered
                             ? "✅ 已启用 — 右键菜单已注册"
                             : "❌ 未启用 — 右键菜单未注册"
                       font.pixelSize: 13
                       color: ctxMenuManager.registered ? "#4CAF50" : Theme.color.onSurfaceVariantColor
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
                           text: "共 " + mainWindow.threatHistory.threatCount + " 条记录"
                           font.pixelSize: 14; color: Theme.color.onSurfaceVariantColor
                       }
                       Item { Layout.fillWidth: true }
                       Button {
                           text: "🔄 刷新"
                           onClicked: mainWindow.threatHistory.refresh()
                       }
                       Button {
                           text: "🗑 清除"
                           enabled: mainWindow.threatHistory.threatCount > 0
                           onClicked: mainWindow.threatHistory.clearHistory()
                       }
                   }

                   ScrollView {
                       Layout.fillWidth: true; Layout.fillHeight: true; clip: true

                       ListView {
                           id: threatList
                           model: mainWindow.threatHistory.threats

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
               text: "数据管理"
               font.pixelSize: 20; font.bold: true
               color: Theme.color.onSurfaceColor
           }

           Card {
               Layout.fillWidth: true; padding: 20

               ColumnLayout {
                   spacing: 12
                   Layout.fillWidth: true

                   Text {
                       text: "重置所有数据"
                       font.pixelSize: 16; font.bold: true
                       color: Theme.color.onSurfaceColor
                   }
                   Text {
                       text: "将清除保护天数、扫描历史、主题偏好和更新日志状态。\n程序将恢复到首次安装时的状态。"
                       font.pixelSize: 13; color: Theme.color.onSurfaceVariantColor
                       Layout.maximumWidth: 500
                       wrapMode: Text.WordWrap
                   }

                   Button {
                       text: "⚠ 重置所有数据"
                       Layout.fillWidth: true
                       Layout.preferredHeight: 44
                       onClicked: resetConfirmDialog.open()
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
                       Text { text: "1.9.0"; color: Theme.color.onSurfaceColor; font.pixelSize: 14 }
                       Text { text: "扫描引擎:"; color: Theme.color.onSurfaceVariantColor; font.pixelSize: 14 }
                       Text { text: "哈希 → ClamAV → YARA → Windows Defender"; color: Theme.color.onSurfaceColor; font.pixelSize: 14 }
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

   // ── Reset confirmation dialog ────────────────────────────
   Dialog {
       id: resetConfirmDialog
       title: "⚠ 确认重置"
       width: 420
       acceptText: "确认重置"
       showRejectButton: true
       rejectText: "取消"

       onAccepted: {
           mainWindow.engineManager.resetData()
           mainWindow.themeManager.resetToDefaults()
           mainWindow.updateManager.resetChangelog()
           mainWindow.showSnackbar("✅ 所有数据已重置！下次启动将如首次安装", "", null)
       }

       content: ColumnLayout {
           spacing: 12
           width: parent.width
           Text {
               text: "确定要重置所有数据吗？"
               font.pixelSize: 16; font.bold: true
               color: Theme.color.onSurfaceColor
           }
           Text {
               text: "此操作将清除：\n• 保护天数记录\n• 扫描历史和上次扫描信息\n• 主题偏好（恢复为跟随系统）\n• 更新日志状态（下次启动重新显示）\n\n程序文件不会被删除。"
               font.pixelSize: 14; color: Theme.color.onSurfaceVariantColor
               wrapMode: Text.WordWrap
               Layout.fillWidth: true
           }
       }
   }

   // ── Color picker dialog ─────────────────────────────────
   ColorDialog {
       id: colorDialog
       title: "选择主题色"
       onAccepted: {
           StyleManager.seedColor = colorDialog.selectedColor;
           mainWindow.themeManager.applySeedColor(colorDialog.selectedColor);
       }
   }
}

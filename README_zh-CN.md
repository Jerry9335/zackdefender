# 🛡 Zack Defender

<p align="center">
  <img src="docs/banner.png" alt="Zack Defender" width="600" />
</p>

<p align="center">
  <b>现代化 Windows 安全中心 · 基于 Windows Defender 引擎</b><br>
  Material Design 3 界面 | 轻量 · 高效 · 开源
</p>

<p align="center">
  <img src="https://img.shields.io/badge/版本-1.2.0-blue" />
  <img src="https://img.shields.io/badge/Qt-6.11-green" />
  <img src="https://img.shields.io/badge/平台-Windows%2010%2F11-lightgrey" />
  <img src="https://img.shields.io/badge/引擎-Windows%20Defender-red" />
  <img src="https://img.shields.io/badge/许可证-MIT-yellow" />
</p>

<p align="center">
  <a href="README.md">EN English</a> | <a href="README_zh-CN.md">CN 简体中文</a>
</p>

---

## ✨ 功能特性

| 功能 | 说明 |
|------|------|
| 🔍 **病毒扫描** | 通过 Windows Defender (MpCmdRun) 执行快速扫描、全盘扫描和自定义文件夹扫描 |
| 🛡 **威胁处理** | 每次扫描后一键隔离或忽略检测到的威胁 |
| 📊 **实时监控** | 展示防护状态、病毒库版本、引擎信息等实时数据 |
| 📋 **威胁历史** | 浏览 Windows Defender 检测到的所有威胁，包含完整详情 |
| 📦 **隔离区管理** | 查看和管理隔离文件 — 支持还原或永久删除 |
| 🔔 **通知系统** | MD3 风格应用内提示 + 系统托盘 Windows 气泡通知 |
| 📌 **系统托盘** | 最小化到托盘，右键菜单快速扫描，守护始终在线 |
| 🎨 **主题切换** | 跟随系统 / 浅色 / 深色模式 — 自动适配 Windows 主题或手动选择 |
| 📋 **更新日志** | 版本更新后首次运行自动弹出，也可从设置页面再次查看 |
| 🔒 **单例运行** | 只允许一个实例运行，重复启动时会友好提示 |

## 📸 截图预览

> 请将截图放入 `docs/` 文件夹，并替换下方链接：

| 仪表板 | 扫描界面 | 深色主题 |
|:---:|:---:|:---:|
| ![](docs/dashboard.png) | ![](docs/scan.png) | ![](docs/theme-dark.png) |

## 🚀 安装指南

从 [Releases](https://github.com/Jerry9335/ZackDefender/releases) 下载最新安装包：

| 文件 | 大小 |
|------|------|
| `Zack-Defender-v1.2.0.exe` | 23.3 MB |

运行安装程序，按照向导完成安装。系统要求：

- **Windows 10** 21H2 (build 19044) 或更高版本 / **Windows 11**
- 系统必须启用 **Windows Defender**

## 🛠 技术栈

| 层次 | 技术 |
|------|------|
| **UI 框架** | Qt 6.11 + QML |
| **设计系统** | Material Design 3 (MD3) |
| **编程语言** | C++17 |
| **构建系统** | CMake + Ninja |
| **编译器** | MinGW 13.1 (GCC) |
| **扫描引擎** | Windows Defender (MpCmdRun.exe + PowerShell) |

**7 个异步 C++ 后端** — 绝不阻塞 UI 线程：

# 🛡 Zack Defender

<p align="center">
  <img src="docs/banner.png" alt="Zack Defender" width="600" />
</p>

<p align="center">
  <b>A modern Windows security center powered by Windows Defender.</b><br>
  基于 Windows Defender 引擎的现代化安全中心 · Material Design 3 界面
</p>

<p align="center">
  <img src="https://img.shields.io/badge/version-1.2.0-blue" />
  <img src="https://img.shields.io/badge/Qt-6.11-green" />
  <img src="https://img.shields.io/badge/platform-Windows%2010%2F11-lightgrey" />
  <img src="https://img.shields.io/badge/engine-Windows%20Defender-red" />
  <img src="https://img.shields.io/badge/license-MIT-yellow" />
</p>

---

## ✨ Features

| Feature | Description |
|--------|-------------|
| 🔍 **Virus Scanning** | Quick, full, and custom folder scans via Windows Defender (MpCmdRun) |
| 🛡 **Threat Handling** | One-click quarantine or ignore detected threats after each scan |
| 📊 **Real-Time Monitor** | Live protection status, signature version, and engine info |
| 📋 **Threat History** | Browse all threats detected by Windows Defender with full details |
| 📦 **Quarantine Manager** | View and manage quarantined files — restore or permanently delete |
| 🔔 **Notifications** | MD3 snackbar in-app + Windows toast bubbles via system tray |
| 📌 **System Tray** | Minimize to tray, right-click for quick scan, always-on protection |
| 🎨 **Themes** | System / Light / Dark mode — follows your Windows theme or manual pick |
| 📋 **Changelog** | Auto popup on first run after update, revisitable from Settings |
| 🔒 **Single Instance** | Only one instance can run — polite notice if you try to open a second |

## 📸 Screenshots

> Add screenshots to a `docs/` folder, then replace the links below:

| Dashboard | Scan | Dark Theme |
|:---:|:---:|:---:|
| ![](docs/dashboard.png) | ![](docs/scan.png) | ![](docs/theme-dark.png) |

## 🚀 Installation

Download the latest installer from [Releases](https://github.com/Zack/ZackDefender/releases):

| File | Size |
|------|------|
| `Zack-Defender-v1.2.0.exe` | 23.3 MB |

Run the installer and follow the wizard. Requires:

- **Windows 10** 21H2 (build 19044) or later / **Windows 11**
- **Windows Defender** must be enabled on the system

## 🛠 Tech Stack

| Layer | Technology |
|-------|-----------|
| **UI Framework** | Qt 6.11 + QML |
| **Design System** | Material Design 3 (MD3) |
| **Language** | C++17 |
| **Build System** | CMake + Ninja |
| **Compiler** | MinGW 13.1 (GCC) |
| **Engine** | Windows Defender (MpCmdRun.exe + PowerShell) |

**7 async C++ backends** — zero UI thread blocking:

```
DefenderScanner   → Scan engine wrapper (MpCmdRun)
ProtectionMonitor → Real-time status (Get-MpComputerStatus)
ThreatHistory     → Historical threats (Get-MpThreatDetection)
QuarantineManager → Local + Defender quarantine
TrayManager       → System tray icon & notifications
ThemeManager      → Dark/Light/System theme
UpdateManager     → Changelog on version bump
```

## 🔧 Building from Source

### Prerequisites

- Qt 6.11+ (mingw_64 kit)
- MinGW 13.1+ (bundled with Qt)
- CMake 3.16+
- [Material Components QML](https://github.com/Zack/material-components-qml) — clone to parent directory

### Build

```bash
# Set up environment
export PATH="/d/Qt/Tools/CMake_64/bin:/d/Qt/Tools/Ninja:/d/Qt/Tools/mingw1310_64/bin:/d/Qt/6.11.1/mingw_64/bin:$PATH"

# Configure & compile
cd ZackDefender
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="/d/Qt/6.11.1/mingw_64"
cmake --build build

# Package for distribution (optional)
deploy.bat
```

## 📁 Project Structure

```
ZackDefender/
├── src/
│   ├── main.cpp              # Entry point + single-instance lock
│   ├── resources.qrc          # Qt resource file
│   ├── backend/               # C++ backends (7 classes)
│   ├── qml/
│   │   ├── Main.qml           # Root window + global components
│   │   └── pages/             # 5 pages
│   └── assets/                # Icon + RC file
├── CMakeLists.txt             # Build configuration
├── deploy.bat                 # One-click portable packaging
├── EULA.txt                   # License agreement
└── qtquickcontrols2.conf      # Font config (Microsoft YaHei)
```

## 📄 License

This project is released under the [MIT License](LICENSE).

Third-party components:
- [Qt Framework](https://www.qt.io/) — LGPL v3 / GPL v3
- [Material Components QML](https://github.com/sudoevolve/material-components-qml) — LGPL v3

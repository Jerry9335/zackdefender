# 🛡 Zack Defender

<p align="center">
  <img src="docs/banner1.png" alt="Zack Defender" width="600" />
</p>

<p align="center">
  <b>A modern Windows security center with four-engine local scanning.</b><br>
  Material Design 3 UI | Lightweight · Efficient · Open Source
</p>

<p align="center">
  <img src="https://img.shields.io/badge/version-1.9.0-blue" />
  <img src="https://img.shields.io/badge/Qt-6.11-green" />
  <img src="https://img.shields.io/badge/platform-Windows%2010%2F11-lightgrey" />
  <img src="https://img.shields.io/badge/engines-Hash%2BClamAV%2BYARA%2BDefender-red" />
  <img src="https://img.shields.io/badge/license-MIT-yellow" />
</p>

<p align="center">
  <a href="README.md">EN English</a> | <a href="README_zh-CN.md">简体中文</a>
</p>

---

## ✨ Features

| Feature | Description |
|--------|-------------|
| 🛡 **Four-Engine Scanning** | Hash → ClamAV → YARA → Windows Defender — progressive scanning across four layers, catching threats that others miss |
| 🔍 **Multiple Scan Modes** | Quick scan (7 critical directories), full system scan, custom folder scan, right-click context menu scan |
| 🦊 **SilverFox Specialized** | Dedicated detection for SilverFox trojan variants prevalent in the Asia-Pacific region |
| 📊 **Real-Time Monitor** | Live protection status, signature version, and engine info via WMI & PowerShell |
| 📋 **Global Threat History** | Cross-page persistent history — scan results and system threats unified in one view |
| 📦 **Quarantine Manager** | View and manage quarantined files — restore or permanently delete |
| 🔔 **Notifications** | MD3 snackbar in-app + Windows toast bubbles via system tray |
| 📌 **System Tray** | Minimize to tray, right-click for quick scan, always-on protection |
| 🎨 **Themes** | System / Light / Dark mode — follows your Windows theme or manual pick |
| 🎭 **Mascot System** | An adorable fox companion through every scan — security doesn't have to be scary |
| 📋 **Changelog** | Auto popup on first run after update, revisitable from Settings |
| 🔒 **Single Instance** | Only one instance can run — polite notice if you try to open a second |
| 📦 **MSIX Package** | Modern installer with automatic updates and clean uninstall |
| 🖼 **Context Menu** | Right-click any file or folder in Explorer to scan instantly |

## 📸 Screenshots

| Dashboard | Scan | Dark Theme | Silver fox |
|:---:|:---:|:---:|:---:|
| ![](docs/dashboard.png) | ![](docs/scan.png) | ![](docs/theme-dark.png) | ![](docs/Silver.png) |

## 🚀 Installation

Download the latest release from [Releases](https://github.com/Jerry9335/ZackDefender/releases):

| File | Format | Notes |
|------|--------|-------|
| `Zack-Defender-v1.9.0.msix` | MSIX | Recommended — modern installer, Win10 21H2+ |

System requirements:

- **Windows 10** 21H2 (build 19044) or later / **Windows 11**
- Windows Defender recommended to keep enabled (serves as the 4th engine layer)
- ClamAV and YARA engines are bundled — no additional installation required

## 🛠 Tech Stack

| Layer | Technology |
|-------|-----------|
| **UI Framework** | Qt 6.11 + QML |
| **Design System** | Material Design 3 (MD3) |
| **Language** | C++17 |
| **Build System** | CMake + Ninja |
| **Compiler** | MinGW 13.1 (GCC) |
| **Packaging** | MSIX / Portable ZIP |

**Four-Engine Scan Architecture** — sequential execution, progressive escalation:

```
EngineManager (scan orchestrator)
    │
    ├── HashEngine     → SHA-256 blacklist matching (fastest)
    ├── ClamAVEngine   → Open-source signature database (191MB+ signatures)
    ├── YaraEngine     → Custom YARA rules (SilverFox / ransomware / miners)
    └── DefenderEngine → Windows Defender CLI interface (system-level fallback)
```

**15 async C++ backends** — zero UI thread blocking:

```
EngineManager       → Four-engine pipeline orchestrator
HashEngine          → SHA-256 hash blacklist fast matching
ClamAVEngine        → ClamAV open-source AV engine wrapper
YaraEngine          → YARA rule engine (SilverFox-specific rules included)
DefenderEngine      → Windows Defender MpCmdRun wrapper
SilverFoxEngine     → SilverFox trojan specialized detection
DefenderScanner     → General scan worker (path traversal, progress stats)
ProtectionMonitor   → Real-time protection status (WMI + PowerShell)
ThreatHistory       → Global threat history (QSettings persistence)
QuarantineManager   → Local + Defender quarantine management
TrayManager         → System tray icon & notifications
ThemeManager        → Dark/Light/System theme
UpdateManager       → Version changelog on update
ContextMenuManager  → Explorer right-click menu integration
VirusTotalEngine    → VirusTotal online lookup (auxiliary reference)
```

## 🔧 Building from Source

### Prerequisites

- Qt 6.11+ (mingw_64 kit)
- MinGW 13.1+ (bundled with Qt)
- CMake 3.16+
- Material Components QML

### Build

```bash
# Set up environment (adjust paths as needed)
export PATH="/d/Qt/Tools/CMake_64/bin:/d/Qt/Tools/Ninja:/d/Qt/Tools/mingw1310_64/bin:/d/Qt/6.11.1/mingw_64/bin:$PATH"

# Configure & compile
cd ZackDefender
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="/d/Qt/6.11.1/mingw_64"
cmake --build build --target appzackdefender

# Run
./build/appzackdefender.exe
```

## 📁 Project Structure

```
md3-guardian/
├── src/
│   ├── main.cpp                  # Entry point + single-instance lock
│   ├── resources.qrc              # Qt resource file
│   ├── backend/                   # C++ backends (15 classes)
│   │   ├── EngineManager.cpp      # Four-engine scan orchestrator
│   │   ├── HashEngine.cpp         # Hash blacklist engine
│   │   ├── ClamAVEngine.cpp       # ClamAV engine
│   │   ├── YaraEngine.cpp         # YARA rule engine
│   │   ├── DefenderEngine.cpp     # Defender engine
│   │   ├── SilverFoxEngine.cpp    # SilverFox specialized detection
│   │   ├── VirusTotalEngine.cpp   # VirusTotal lookup
│   │   ├── DefenderScanner.cpp    # General scan worker
│   │   ├── ProtectionMonitor.cpp  # Real-time protection monitor
│   │   ├── ThreatHistory.cpp      # Global threat history
│   │   ├── QuarantineManager.cpp  # Quarantine manager
│   │   ├── TrayManager.cpp        # System tray
│   │   ├── ThemeManager.cpp       # Theme manager
│   │   ├── UpdateManager.cpp      # Changelog manager
│   │   └── ContextMenuManager.cpp # Right-click menu
│   ├── qml/
│   │   ├── Main.qml               # Root window + global components
│   │   └── pages/
│   │       ├── DashboardPage.qml  # Home dashboard
│   │       ├── ScanPage.qml       # Scan center
│   │       ├── ProtectionPage.qml # Protection status
│   │       ├── QuarantinePage.qml # Quarantine
│   │       ├── SilverFoxPage.qml  # SilverFox specialized
│   │       └── SettingsPage.qml   # Settings
│   └── assets/                    # Icons + mascot assets
├── CMakeLists.txt                 # CMake configuration
├── deploy/                        # Engine deployment files
│   ├── engines/
│   │   ├── clamav/                # ClamAV database (~191MB)
│   │   └── yara/                  # YARA rules
│   └── ...
└── qtquickcontrols2.conf          # Font config (Microsoft YaHei)
```

## 📋 Version History

| Version | Date | Highlights |
|---------|------|------------|
| **v1.9.0** | Jul 2026 | 🎨 Visual refresh & UX upgrade: Qt 6.11 build, MSIX packaging, mascot system, global ThreatHistory, emoji vitality |
| v1.8.0 | Internal | 🏗 UI restructure (internal, unreleased) |
| v1.7.0 | Internal | 🔧 Engine replacement: VirusTotal online retired, ClamAV + YARA local engines added |
| v1.6.0 | Internal | 🦊 SilverFox trojan specialized module |
| v1.5.0 | Internal | 🎨 Scan refactor & UI upgrade |
| v1.2.0 | Jun 2026 | 🌐 Initial public release: Windows Defender single engine |

## 📄 License

This project is released under the [MIT License](docs/EULA.txt).

Third-party components:
- [Qt Framework](https://www.qt.io/) — LGPL v3 / GPL v3
- [Material Components QML](https://github.com/sudoevolve/material-components-qml) — LGPL v3
- [ClamAV](https://www.clamav.net/) — GPL v2
- [YARA](https://virustotal.github.io/yara/) — BSD 3-Clause

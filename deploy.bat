@echo off
REM ============================================================
REM  Zack Defender - 一键构建并部署便携版
REM  运行此脚本会自动编译并生成 deploy/ 文件夹
REM  将 deploy/ 文件夹复制到任何 Windows 10/11 电脑即可运行
REM ============================================================

echo [1/5] 设置编译环境...
set "PATH=D:\Qt\Tools\CMake_64\bin;D:\Qt\Tools\Ninja;D:\Qt\Tools\mingw1310_64\bin;D:\Qt\6.11.1\mingw_64\bin;%PATH%"

echo [2/5] 编译项目...
cd /d "%~dp0"
cmake --build build --config Release
if %ERRORLEVEL% NEQ 0 (
    echo ❌ 编译失败！
    pause
    exit /b 1
)

echo [3/5] 准备部署目录...
if exist deploy rmdir /s /q deploy
mkdir deploy
copy build\\appzackdefender.exe deploy\\
copy qtquickcontrols2.conf deploy\\
copy src\\assets\\app_icon.ico deploy\\

:: ── 创建 .firstrun 标记文件 — 安装后首次运行强制弹出更新日志 ──
type nul > deploy\\.firstrun

:: 生成 qt.conf — 告诉 Qt 去哪里找插件和 QML 模块
(
echo [Paths]
echo Plugins = .
echo Imports = qml
) > deploy\qt.conf

echo [4/5] 运行 windeployqt...
windeployqt deploy\appzackdefender.exe --qmldir src\qml --no-translations --no-system-d3d-compiler --no-opengl-sw
if %ERRORLEVEL% NEQ 0 (
    echo ⚠ windeployqt 返回非零，检查输出
)

echo [5/5] 复制 md3Core 组件...
if exist build\qml\md3\Core (
    xcopy /e /y build\qml\md3\Core deploy\qml\md3\Core\ > nul
    :: 修复 qmldir: 部署版用文件系统路径而非资源路径
    powershell -Command "(Get-Content deploy\qml\md3\Core\qmldir) -replace 'prefer :/md3/Core/', 'prefer .' | Set-Content deploy\qml\md3\Core\qmldir"
) else (
    echo ⚠ 未找到 build\qml\md3\Core，请先编译！
)

echo.
echo ✅ 部署完成！
echo 📁 便携版位置: %~dp0deploy\
echo 🚀 将 deploy 文件夹复制到任意 Windows 10/11 电脑，运行 启动ZackDefender.bat 即可！
echo 📦 部署大小: 
dir deploy /s | find "个文件"

pause

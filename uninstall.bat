@echo off
REM ============================================================
REM  Zack Defender - 完全卸载脚本
REM  清除注册表数据 + 删除程序文件夹
REM ============================================================
echo === Zack Defender 完全卸载 ===
echo.

REM ── 1. 清除注册表数据 ──────────────────────────────
echo [1/2] 清除注册表数据...
reg delete "HKCU\Software\ZackDefender" /f > nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo   ✅ 注册表数据已清除
) else (
    echo   ⚠ 注册表数据不存在或已清除
)

REM ── 2. 删除程序文件夹 ──────────────────────────────
echo [2/2] 删除程序文件夹...
set "DEPLOY_DIR=%~dp0deploy"
if exist "%DEPLOY_DIR%" (
    rmdir /s /q "%DEPLOY_DIR%"
    echo   ✅ 程序文件夹已删除
) else (
    echo   ⚠ 程序文件夹不存在
)

echo.
echo ✅ 卸载完成！Zack Defender 已从本机完全清除。
echo    （如果正在运行，请手动退出系统托盘中的程序）
pause

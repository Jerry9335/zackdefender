@echo off
setlocal enabledelayedexpansion
title 银狐病毒紧急清理 @ZackDefender
color 2f
fltmc >nul 2>&1
if %errorlevel% neq 0 (
    powershell -Command "Start-Process cmd -ArgumentList '/c \"%~s0\" %*' -Verb RunAs"
    exit /b
)
cd /d "%~dp0"

echo.
echo ================================================
echo        银狐病毒紧急清理工具
echo        Zack Defender 集成版
echo ================================================
echo.
echo 此脚本将执行以下操作：
echo   1. 移除 WDAC 代码完整性策略
echo   2. 清理已知恶意服务注册表
echo   3. 禁用根目录下所有任务计划
echo   4. 劫持并终止可疑进程（保留关键系统进程）
echo   5. 禁用可疑服务
echo   6. 清理可疑 DLL 文件
echo.
echo 警告：执行后必须立即重启计算机！
echo       重启后请使用杀毒软件全盘扫描！
echo.
pause

echo [1/6] 移除 WDAC 策略...
takeown /f C:\Windows\System32\CodeIntegrity\SiPolicy.p7b >nul 2>&1
icacls C:\Windows\System32\CodeIntegrity\SiPolicy.p7b /grant administrators:F >nul 2>&1
del /f /q C:\Windows\System32\CodeIntegrity\SiPolicy.p7b >nul 2>&1
if exist "C:\Windows\System32\CodeIntegrity\CiPolicies\Active" (
    del /s /f /q C:\Windows\System32\CodeIntegrity\CiPolicies\Active\* >nul 2>&1
)
echo [OK] WDAC 策略已移除

echo [2/6] 清理已知恶意服务注册表...
set BAD_KEYS=vafdska MiniFilterDrv vmservice MicrosoftSoftware2ShadowCop4yProvider
for %%k in (%BAD_KEYS%) do (
    reg delete "HKLM\SYSTEM\ControlSet001\Services\%%k" /f >nul 2>&1
)
echo [OK] 已知恶意服务注册表已清理

echo [3/6] 清理可疑 DLL...
if exist "C:\Windows\System32\wow64log.dll" (
    del /f /q "C:\Windows\System32\wow64log.del" >nul 2>&1
    ren "C:\Windows\System32\wow64log.dll" wow64log.del >nul 2>&1
    echo [OK] wow64log.dll 已重命名
) else (
    echo [跳过] 未发现 wow64log.dll
)

echo [4/6] 禁用根目录任务计划...
powershell -Command "Get-ScheduledTask -TaskPath '\' -ErrorAction SilentlyContinue | Disable-ScheduledTask -ErrorAction SilentlyContinue" >nul 2>&1
echo [OK] 任务计划已禁用

echo [5/6] 劫持并终止可疑进程...
set IFEO=HKLM\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Image File Execution Options
set WL=explorer.exe svchost.exe lsass.exe winlogon.exe csrss.exe services.exe dllhost.exe smss.exe dwm.exe wininit.exe conhost.exe cmd.exe powershell.exe regedit.exe taskmgr.exe

for /f "delims=" %%p in ('powershell -Command "$list=Get-WmiObject Win32_Process|Where-Object{$_.ExecutablePath -ne $null};$list.ExecutablePath|Sort-Object -Unique" 2^>nul') do (
    set "fp=%%p"
    if exist "!fp!" (
        set "attr=%%~ap"
        echo !attr! | findstr /i "h s" >nul
        if !errorlevel! equ 0 (
            for %%e in ("!fp!") do set "ename=%%~nxe"
            set "skip=0"
            for %%w in (!WL!) do if /i "!ename!"=="%%w" set "skip=1"
            if !skip! equ 0 (
                echo [劫持] !ename!
                reg add "!IFEO!\!ename!" /v Debugger /t REG_SZ /d "ZackDefender" /f >nul 2>&1
                taskkill /f /im "!ename!" >nul 2>&1
            ) else (
                echo [跳过-系统进程] !ename!
            )
        )
    )
)
echo [OK] 进程处理完成

echo [6/6] 禁用可疑服务...
for /f "delims=" %%a in ('reg query "HKLM\SYSTEM\CurrentControlSet\Services" /s /v ImagePath 2^>nul') do (
    set "line=%%a"
    echo !line! | findstr /i /C:"cmd /c" /C:"powershell" /C:"wscript" /C:"cscript" /C:"mshta" /C:"pwsh" >nul
    if !errorlevel! equ 0 (
        set "svckey=!current_key!"
        reg add "!svckey!" /v Start /t REG_DWORD /d 4 /f >nul 2>&1
        echo [禁用] !svckey!
    )
    echo !line! | findstr /i "ImagePath" >nul
    if !errorlevel! neq 0 set "current_key=!line!"
)

echo.
echo ================================================
echo 清理完成！
echo 请立即重启计算机，然后安装/运行杀毒软件全盘扫描。
echo.
echo 推荐：360杀毒 https://sd.360.cn
echo       火绒安全 https://www.huorong.cn
echo ================================================
pause

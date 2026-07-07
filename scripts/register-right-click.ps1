# ============================================================
#  Zack Defender - 注册右键扫描菜单
#  以管理员身份运行此脚本
# ============================================================
#Requires -RunAsAdministrator

$ErrorActionPreference = "Stop"

# ── 自动查找 exe 路径 ──────────────────────────────
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$projectRoot = Split-Path -Parent $scriptDir

# 优先级: deploy/ > build/ > 手动输入
$exeCandidates = @(
    "$projectRoot\deploy\appzackdefender.exe",
    "$projectRoot\build\appzackdefender.exe"
)

$exePath = $null
foreach ($candidate in $exeCandidates) {
    if (Test-Path $candidate) {
        $exePath = $candidate
        break
    }
}

if (-not $exePath) {
    $exePath = Read-Host "未找到 appzackdefender.exe，请输入完整路径"
    if (-not (Test-Path $exePath)) {
        Write-Host "❌ 路径不存在: $exePath" -ForegroundColor Red
        exit 1
    }
}

Write-Host "📌 使用: $exePath" -ForegroundColor Cyan
Write-Host ""

$menuName = "ZackDefenderScan"
$displayName = "使用 Zack Defender 扫描"
$command = "`"$exePath`" `"%1`""

# ── 注册表路径 ────────────────────────────────────
$paths = @(
    "Registry::HKEY_CLASSES_ROOT\*\shell\$menuName",        # 所有文件
    "Registry::HKEY_CLASSES_ROOT\Directory\shell\$menuName", # 文件夹
    "Registry::HKEY_CLASSES_ROOT\Drive\shell\$menuName"      # 驱动器
)

foreach ($regPath in $paths) {
    Write-Host "📝 注册: $regPath"

    # 创建主键 + 显示名称 + 图标
    New-Item -Path $regPath -Force | Out-Null
    Set-ItemProperty -Path $regPath -Name "(Default)" -Value $displayName
    Set-ItemProperty -Path $regPath -Name "Icon" -Value $exePath

    # 创建 command 子键
    $cmdPath = "$regPath\command"
    New-Item -Path $cmdPath -Force | Out-Null
    Set-ItemProperty -Path $cmdPath -Name "(Default)" -Value $command
}

Write-Host ""
Write-Host "✅ 右键菜单注册完成！" -ForegroundColor Green
Write-Host ""
Write-Host "现在可以在文件资源管理器中右键点击文件/文件夹/驱动器 → '$displayName'" -ForegroundColor Yellow
Write-Host ""
Write-Host "💡 提示: 如需卸载右键菜单，运行 unregister-right-click.ps1" -ForegroundColor Gray

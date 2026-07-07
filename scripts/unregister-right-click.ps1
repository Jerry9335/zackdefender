# ============================================================
#  Zack Defender - 移除右键扫描菜单
#  以管理员身份运行此脚本
# ============================================================
#Requires -RunAsAdministrator

$ErrorActionPreference = "Continue"

$menuName = "ZackDefenderScan"

$paths = @(
    "Registry::HKEY_CLASSES_ROOT\*\shell\$menuName",
    "Registry::HKEY_CLASSES_ROOT\Directory\shell\$menuName",
    "Registry::HKEY_CLASSES_ROOT\Drive\shell\$menuName"
)

foreach ($regPath in $paths) {
    Write-Host "🗑 删除: $regPath"
    Remove-Item -Path $regPath -Recurse -Force -ErrorAction SilentlyContinue
}

Write-Host ""
Write-Host "✅ 右键菜单已移除！" -ForegroundColor Green

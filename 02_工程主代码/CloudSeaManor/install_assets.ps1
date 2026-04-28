# Cloud Sea Manor - Asset Installer
# ==========================================
# 下载游戏所需的第三方像素美术资源（Kenney Tiny Town CC0 许可）。
# 运行方式：以管理员身份在 PowerShell 中执行：
#   .\install_assets.ps1
#
# 或者直接双击运行。

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  云海山庄 - 资源安装程序" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

$ErrorActionPreference = "Stop"
$BaseDir = Split-Path -Parent $MyInvocation.MyCommand.Path

# Kenney Tiny Town (CC0 - 公共领域)
# https://kenney.nl/assets/tiny-town
# GitHub 镜像: https://github.com/nicbou/kenney-assets
$TinyTownUrl = "https://github.com/nicbou/kenney-assets/raw/refs/heads/master/assets/tiny-town/Tilemap/tilemap_packed.png"
$TextureDst = Join-Path $BaseDir "assets\textures\third_party\kenney_tiny-town\Tilemap"
$TextureDstFile = Join-Path $TextureDst "tilemap_packed.png"

# ui_main.atlas.json → assets/textures/third_party/kenney_tiny-town/Tilemap/tilemap_packed.png (192x176)
# player_main.atlas.json → 同上
# items_crop.atlas.json → 同上

Write-Host "[1/3] 创建目录结构..." -ForegroundColor Yellow
New-Item -ItemType Directory -Path $TextureDst -Force | Out-Null
Write-Host "      目录: $TextureDst" -ForegroundColor Gray

Write-Host "[2/3] 下载 Kenney Tiny Town tilemap_packed.png..." -ForegroundColor Yellow
Write-Host "      来源: $TinyTownUrl" -ForegroundColor Gray
Write-Host "      目标: $TextureDstFile" -ForegroundColor Gray

try {
    Invoke-WebRequest -Uri $TinyTownUrl -OutFile $TextureDstFile -TimeoutSec 30 -UserAgent "Mozilla/5.0"
    $fileSize = (Get-Item $TextureDstFile).Length
    Write-Host "      下载成功！文件大小: $fileSize bytes" -ForegroundColor Green
} catch {
    Write-Host "      下载失败: $_" -ForegroundColor Red
    Write-Host "      请手动下载文件并放置到:" -ForegroundColor Yellow
    Write-Host "      $TextureDstFile" -ForegroundColor Yellow
}

Write-Host ""
Write-Host "[3/3] 验证资源..." -ForegroundColor Yellow
if (Test-Path $TextureDstFile) {
    $size = (Get-Item $TextureDstFile).Length
    if ($size -gt 1000) {
        Write-Host "      tilemap_packed.png 已就绪 ($size bytes)" -ForegroundColor Green
    } else {
        Write-Host "      文件太小，可能下载失败" -ForegroundColor Red
    }
} else {
    Write-Host "      文件不存在" -ForegroundColor Red
}

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  安装完成！" -ForegroundColor Cyan
Write-Host "  请重新编译并运行游戏。" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "提示: 如果下载持续失败，可手动从以下地址下载:" -ForegroundColor Yellow
Write-Host "  https://kenney.nl/assets/tiny-town" -ForegroundColor Gray
Write-Host "  将 Tilemap/tilemap_packed.png 放入:" -ForegroundColor Gray
Write-Host "  $TextureDst" -ForegroundColor Gray

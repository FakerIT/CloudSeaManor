# Generate placeholder pixel art PNG for atlas textures
# Uses System.Drawing to create a minimal valid PNG when Kenney assets are unavailable
Write-Host "Generating placeholder PNG assets..." -ForegroundColor Cyan

Add-Type -AssemblyName System.Drawing

function New-PlaceholderSpriteSheet {
    param(
        [string]$Path,
        [int]$Width = 192,
        [int]$Height = 176,
        [string]$Name = "placeholder"
    )

    # Create directory
    $dir = Split-Path -Parent $Path
    if (-not (Test-Path $dir)) {
        New-Item -ItemType Directory -Path $dir -Force | Out-Null
    }

    # Create bitmap
    $bmp = New-Object System.Drawing.Bitmap($Width, $Height)
    $g = [System.Drawing.Graphics]::FromImage($bmp)

    # Fill with neutral dark color (pixel art grid background)
    $bgBrush = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(255, 48, 40, 32))
    $g.FillRectangle($bgBrush, 0, 0, $Width, $Height)

    # Draw grid pattern (simulating pixel art atlas)
    $linePen = New-Object System.Drawing.Pen([System.Drawing.Color]::FromArgb(255, 70, 58, 46), 1)
    $gridSize = 16

    # Horizontal lines
    for ($y = 0; $y -lt $Height; $y += $gridSize) {
        $g.DrawLine($linePen, 0, $y, $Width, $y)
    }
    # Vertical lines
    for ($x = 0; $x -lt $Width; $x += $gridSize) {
        $g.DrawLine($linePen, $x, 0, $x, $Height)
    }

    # Draw some placeholder icons
    $iconBrushes = @(
        (New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(255, 220, 180, 60)),   # gold/coin
        (New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(255, 80, 200, 120)),   # green/hearts
        (New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(255, 100, 160, 240)),  # blue/cloud
        (New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(255, 220, 80, 80)),   # red/quest
        (New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(255, 180, 120, 60))  # brown/wood
    )

    $brushIdx = 0
    for ($y = 16; $y -lt $Height; $y += 32) {
        for ($x = 0; $x -lt $Width; $x += 32) {
            $brush = $iconBrushes[$brushIdx % $iconBrushes.Length]
            $g.FillRectangle($brush, $x + 4, $y + 4, 24, 24)
            $brushIdx++
        }
    }

    # Add border
    $borderPen = New-Object System.Drawing.Pen([System.Drawing.Color]::FromArgb(255, 120, 90, 60), 2)
    $g.DrawRectangle($borderPen, 0, 0, $Width - 1, $Height - 1)

    # Label
    $font = New-Object System.Drawing.Font("Consolas", 8)
    $labelBrush = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(255, 200, 180, 140))
    $g.DrawString("PLACEHOLDER", $font, $labelBrush, 4, 4)

    # Save
    $bmp.Save($Path, [System.Drawing.Imaging.ImageFormat]::Png)
    $g.Dispose()
    $bmp.Dispose()

    $fileSize = (Get-Item $Path).Length
    Write-Host "  Created: $Path ($fileSize bytes)" -ForegroundColor Green
}

$BaseDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$TinyTownDir = Join-Path $BaseDir "assets\textures\third_party\kenney_tiny-town\Tilemap"

# Main atlas: 192x176 (matches ui_main.atlas.json)
New-PlaceholderSpriteSheet -Path (Join-Path $TinyTownDir "tilemap_packed.png") `
    -Width 192 -Height 176 -Name "tilemap_packed"

Write-Host ""
Write-Host "Done. These are placeholder textures - download real assets for production." -ForegroundColor Yellow
Write-Host "See install_assets.ps1 to download Kenney CC0 assets automatically." -ForegroundColor Gray

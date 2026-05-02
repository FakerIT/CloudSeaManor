param(
    [switch]$SkipBuild
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

function Write-Step {
    param([string]$Message)
    Write-Host "==> $Message" -ForegroundColor Cyan
}

function Ensure-Directory {
    param([string]$Path)
    if (-not (Test-Path -LiteralPath $Path)) {
        New-Item -ItemType Directory -Path $Path | Out-Null
    }
}

function Remove-IfLink {
    param([string]$Path)
    if (-not (Test-Path -LiteralPath $Path)) {
        return $true
    }

    $item = Get-Item -LiteralPath $Path -Force
    if (($item.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
        Remove-Item -LiteralPath $Path -Force
        return $true
    }

    return $false
}

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
$sourceGstack = Join-Path $repoRoot "skills/gstack"
$cursorSkillsRoot = Join-Path $repoRoot ".cursor/skills"
$targetGstack = Join-Path $cursorSkillsRoot "gstack"

if (-not (Test-Path -LiteralPath $sourceGstack)) {
    throw "Source skill folder not found: $sourceGstack"
}

Write-Step "Preparing project skill directories"
Ensure-Directory -Path $cursorSkillsRoot

Write-Step "Syncing gstack into .cursor/skills/gstack"
Ensure-Directory -Path $targetGstack
Copy-Item -Path (Join-Path $sourceGstack "*") -Destination $targetGstack -Recurse -Force

$skillDirs = Get-ChildItem -Path $targetGstack -Directory | Where-Object {
    Test-Path -LiteralPath (Join-Path $_.FullName "SKILL.md")
}

Write-Step "Creating skill entry links under .cursor/skills"
foreach ($dir in $skillDirs) {
    $skillName = $dir.Name
    $linkPath = Join-Path $cursorSkillsRoot $skillName
    $targetAbsolute = Join-Path $targetGstack $skillName

    if (Test-Path -LiteralPath $linkPath) {
        Remove-Item -LiteralPath $linkPath -Recurse -Force
    }
    Copy-Item -Path $targetAbsolute -Destination $linkPath -Recurse -Force
}

if ($SkipBuild) {
    Write-Step "SkipBuild enabled, deployment completed without build"
    exit 0
}

$bunCmd = Get-Command bun -ErrorAction SilentlyContinue
if (-not $bunCmd) {
    Write-Warning "Bun is not installed or not in PATH. Deployment is done, but browse binary is not built."
    Write-Host "Install bun, then run: powershell -ExecutionPolicy Bypass -File .\scripts\setup-project-skills.ps1"
    exit 0
}

Write-Step "Installing dependencies"
Push-Location $targetGstack
try {
    & bun install

    Write-Step "Building browse binary"
    & bun run gen:skill-docs
    & bun build --compile browse/src/cli.ts --outfile browse/dist/browse
    & bun build --compile browse/src/find-browse.ts --outfile browse/dist/find-browse
    $versionFile = Join-Path $targetGstack "browse/dist/.version"
    Set-Content -LiteralPath $versionFile -Value "project-vendored" -NoNewline

    Write-Step "Checking Playwright Chromium shell"
    $dryRunOutput = & bunx playwright install chromium --only-shell --dry-run
    $installLine = $dryRunOutput | Where-Object { $_ -match "Install location:\s*(.+)$" } | Select-Object -First 1
    $installPath = $null
    if ($installLine -and $installLine -match "Install location:\s*(.+)$") {
        $installPath = $Matches[1].Trim()
    }

    $expectedExe = $null
    if ($installPath) {
        $expectedExe = Join-Path $installPath "chrome-headless-shell-win64/chrome-headless-shell.exe"
    }

    $localShellDir = Join-Path $repoRoot "05_参考与第三方资源\02_浏览器自动化工具\chrome-headless-shell-win64"
    $localShellExe = Join-Path $localShellDir "chrome-headless-shell.exe"

    if ($expectedExe -and -not (Test-Path -LiteralPath $expectedExe) -and (Test-Path -LiteralPath $localShellExe)) {
        Write-Step "Reusing local chrome-headless-shell-win64"
        Ensure-Directory -Path $installPath
        Copy-Item -Path $localShellDir -Destination (Join-Path $installPath "chrome-headless-shell-win64") -Recurse -Force
    }

    if (-not ($expectedExe -and (Test-Path -LiteralPath $expectedExe))) {
        Write-Step "Installing Playwright Chromium shell"
        & bunx playwright install chromium --only-shell
    }
}
finally {
    Pop-Location
}

$browseExe = Join-Path $targetGstack "browse/dist/browse.exe"
$browseBin = Join-Path $targetGstack "browse/dist/browse"

Write-Step "Verifying browse binary"
if (Test-Path -LiteralPath $browseExe) {
    & $browseExe status
}
elseif (Test-Path -LiteralPath $browseBin) {
    & $browseBin status
}
else {
    throw "Build completed but browse binary was not found in browse/dist/"
}

Write-Host ""
Write-Host "Project skills deployed successfully."
Write-Host "You can now use: /gstack, /qa, /review, /ship, /browse ..."

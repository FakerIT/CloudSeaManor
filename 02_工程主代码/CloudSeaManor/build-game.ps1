# =============================================================================
# 云海山庄 — 统一构建脚本 (One-Click Build)
# =============================================================================
# 用法:
#   .\build-game.ps1          # 完整构建（Debug + 自动启动游戏）
#   .\build-game.ps1 Build    # 仅构建
#   .\build-game.ps1 Test    # 构建测试并运行测试
#   .\build-game.ps1 Clean   # 清理构建产物
#   .\build-game.ps1 Rebuild # 清理后重新构建
#   .\build-game.ps1 Release # Release 模式构建
#   .\build-game.ps1 Run     # 直接运行游戏（不构建）
#   .\build-game.ps1 Status  # 显示工具链状态
# =============================================================================

param(
    [ValidateSet("Build", "Test", "Clean", "Rebuild", "Release", "Run", "Status", "Help")]
    [string]$Action = "Build",

    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Debug",

    [switch]$NoLaunch       # 构建后不自动启动游戏
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

# ============================================================================
# 路径配置
# ============================================================================
$RepoRoot = Split-Path -Parent $PSScriptRoot
$ProjectRoot = $PSScriptRoot  # 脚本所在目录（CloudSeaManor 工程根目录）
$EngineRoot = "$RepoRoot\02_工程主代码\CloudSeaManor"
$SFMLRoot = "$RepoRoot\05_参考与第三方资源\01_SFML引擎\SFML-3.0.2"
$BuildDir = "$EngineRoot\build"
$GameExe = "$BuildDir\CloudSeamanor_game\Debug\CloudSeamanor_game.exe"
$GameExeRelease = "$BuildDir\CloudSeamanor_game\Release\CloudSeamanor_game.exe"

if ($Configuration -eq "Release") {
    $GameExe = $GameExeRelease
}

# ============================================================================
# 颜色输出
# ============================================================================
function Write-Step { param([string]$Msg) Write-Host "== $Msg" -ForegroundColor Cyan }
function Write-Success { param([string]$Msg) Write-Host "  [OK] $Msg" -ForegroundColor Green }
function Write-Warn { param([string]$Msg) Write-Host "  [!] $Msg" -ForegroundColor Yellow }
function Write-Fail { param([string]$Msg) Write-Host "  [X] $Msg" -ForegroundColor Red }
function Write-Info { param([string]$Msg) Write-Host "      $Msg" -ForegroundColor DarkGray }

# ============================================================================
# 工具链检测
# ============================================================================
function Get-Toolchain {
    $result = @{
        VSInstallDir = $null
        MSBuild = $null
        VCVARS = $null
        CMake = $null
        Ninja = $null
    }

    # Visual Studio (vswhere)
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $vswhere) {
        $installPath = & $vswhere -latest -products * -requires Microsoft.Component.MSBuild -property installationPath
        if ($installPath) {
            $result.VSInstallDir = $installPath.Trim()
            $result.VCVARS = Join-Path $installPath "VC\Auxiliary\Build\vcvarsall.bat"
            $result.MSBuild = Join-Path $installPath "MSBuild\Current\Bin\MSBuild.exe"
            if (-not (Test-Path $result.MSBuild)) {
                $result.MSBuild = Join-Path $installPath "MSBuild\Current\Bin\amd64\MSBuild.exe"
            }
            if (-not (Test-Path $result.MSBuild)) {
                $result.MSBuild = Join-Path $installPath "MSBuild\15.0\Bin\MSBuild.exe"
            }
        }
    }

    # CMake
    $cmakePaths = @(
        "C:\Program Files\CMake\bin\cmake.exe",
        "C:\Program Files (x86)\Microsoft Visual Studio\Shared\Common\IDE\Extentions\Microsoft\CMake\CMake\bin\cmake.exe",
        "$($result.VSInstallDir)\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
    )
    foreach ($p in $cmakePaths) {
        if (Test-Path $p) {
            $result.CMake = $p
            break
        }
    }

    # Ninja (optional)
    $ninjaPath = "C:\Program Files\Ninja\cmake.exe"
    # Actually ninja is named ninja.exe, not cmake.exe
    $ninjaExe = "C:\Program Files\Ninja\ninja.exe"
    if (Test-Path $ninjaExe) {
        $result.Ninja = $ninjaExe
    }

    return $result
}

# ============================================================================
# 状态显示
# ============================================================================
function Show-Status {
    $tc = Get-Toolchain
    Write-Host ""
    Write-Host "  云海山庄 — 工具链状态" -ForegroundColor White
    Write-Host "  " -NoNewline
    Write-Host ("=" * 50) -ForegroundColor Gray

    Write-Host "  Visual Studio : " -NoNewline
    if ($tc.VSInstallDir) {
        Write-Host $tc.VSInstallDir -ForegroundColor Green
    } else {
        Write-Host "未找到" -ForegroundColor Red
    }

    Write-Host "  MSBuild.exe  : " -NoNewline
    if ($tc.MSBuild) {
        Write-Host $tc.MSBuild -ForegroundColor Green
    } else {
        Write-Host "未找到" -ForegroundColor Red
    }

    Write-Host "  VCVARS       : " -NoNewline
    if ($tc.VCVARS -and (Test-Path $tc.VCVARS)) {
        Write-Host $tc.VCVARS -ForegroundColor Green
    } else {
        Write-Host "未找到" -ForegroundColor Red
    }

    Write-Host "  CMake        : " -NoNewline
    if ($tc.CMake) {
        $cmakeVer = & $tc.CMake --version 2>$null | Select-Object -First 1
        Write-Host "$($tc.CMake) ($cmakeVer)" -ForegroundColor Green
    } else {
        Write-Host "未找到" -ForegroundColor Red
    }

    Write-Host "  Ninja        : " -NoNewline
    if ($tc.Ninja) {
        Write-Host $tc.Ninja -ForegroundColor Green
    } else {
        Write-Host "未安装（可选）" -ForegroundColor Yellow
    }

    Write-Host "  SFML 3.0.2   : " -NoNewline
    if (Test-Path "$SFMLRoot\include\SFML\Graphics.hpp") {
        Write-Host $SFMLRoot -ForegroundColor Green
    } else {
        Write-Host "未找到，请检查 05_参考与第三方资源/01_SFML引擎/" -ForegroundColor Red
    }

    Write-Host "  游戏可执行文件 : " -NoNewline
    if (Test-Path $GameExe -ErrorAction SilentlyContinue) {
        Write-Host $GameExe -ForegroundColor Green
    } elseif (Test-Path $GameExeRelease -ErrorAction SilentlyContinue) {
        Write-Host $GameExeRelease -ForegroundColor Green
    } else {
        Write-Host "未构建（运行 Build 生成）" -ForegroundColor Yellow
    }

    Write-Host ""
}

# ============================================================================
# MSBuild 构建（vcxproj 方式）
# ============================================================================
function Build-MSBuild {
    param([string]$Config)

    $tc = Get-Toolchain
    if (-not $tc.MSBuild) {
        throw "MSBuild 未找到。请安装 Visual Studio 2022 并确保勾选 C++ 工作负载。"
    }
    if (-not (Test-Path $tc.VCVARS)) {
        throw "vcvarsall.bat 未找到: $($tc.VCVARS)"
    }

    $sln = "$EngineRoot\CloudSeamanor_game.sln"
    if (-not (Test-Path $sln)) {
        throw "解决方案文件未找到: $sln"
    }

    Write-Step "构建 CloudSeamanor（MSBuild / $Config）"
    Write-Info "解决方案: $sln"

    # 加载 vcvars
    $vcvarsScript = @"
call "$($tc.VCVARS)" x64 >nul 2>&1
"$($tc.MSBuild)" "$sln" /t:Build /p:Configuration=$Config /p:Platform=x64 /v:m /nologo
"@

    $batchFile = "$env:TEMP\build_cloudseamanor_$PID.bat"
    $vcvarsBat = "$env:TEMP\vcvars_$PID.bat"

    try {
        # 写入 vcvars 批处理
        "@echo off`ncall `"$($tc.VCVARS)`" x64 >nul 2>&1`n" | Out-File -FilePath $vcvarsBat -Encoding ASCII -Force

        # 写入构建脚本
        "@echo off`ncall `"$vcvarsBat`"`n`"$($tc.MSBuild)`" `"$sln`" /t:Build /p:Configuration=$Config /p:Platform=x64 /v:m /nologo`n" |
            Out-File -FilePath $batchFile -Encoding ASCII -Force

        $process = Start-Process cmd -ArgumentList "/c `"$batchFile`"" -NoNewWindow -Wait -PassThru
        if ($process.ExitCode -ne 0) {
            throw "MSBuild 构建失败 (exit code: $($process.ExitCode))"
        }

        # 查找输出 exe
        $outExe = "$BuildDir\CloudSeamanor_game\$Config\CloudSeamanor_game.exe"
        if (Test-Path $outExe) {
            Write-Success "构建成功: $outExe"
            return $outExe
        } else {
            Write-Warn "未找到预期输出: $outExe"
            return $null
        }
    }
    finally {
        Remove-Item $batchFile -Force -ErrorAction SilentlyContinue
        Remove-Item $vcvarsBat -Force -ErrorAction SilentlyContinue
    }
}

# ============================================================================
# CMake 构建（推荐方式）
# ============================================================================
function Build-CMake {
    param([string]$Config)

    $tc = Get-Toolchain
    if (-not $tc.CMake) {
        throw "CMake 未找到。请安装 CMake 3.23+ 并确保添加到 PATH。"
    }

    Write-Step "构建 CloudSeamanor（CMake / $Config）"
    Write-Info "项目目录: $EngineRoot"
    Write-Info "构建目录: $BuildDir"

    if (-not (Test-Path "$SFMLRoot\include\SFML\Graphics.hpp")) {
        throw "SFML 头文件未找到: $SFMLRoot\include\SFML\Graphics.hpp"
    }

    # 清理旧构建
    if (Test-Path $BuildDir) {
        Write-Info "清理旧构建目录..."
        Remove-Item $BuildDir -Recurse -Force -ErrorAction SilentlyContinue
    }

    # CMake 配置
    $gen = if ($tc.Ninja) { "Ninja" } else { "Visual Studio 17 2022" }
    Write-Info "生成器: $gen"

    $env:SFML_ROOT = $SFMLRoot
    $cmakeArgs = @(
        "-S", $EngineRoot,
        "-B", $BuildDir,
        "-G", $gen,
        "-DCMAKE_BUILD_TYPE=$Config",
        "-DCLOUDSEAMANOR_ENABLE_WARNINGS=ON"
    )
    if ($gen -eq "Ninja") {
        $cmakeArgs += @("-DCMAKE_INSTALL_PREFIX=$EngineRoot\out\install")
    }

    Write-Info "CMake 参数: $($cmakeArgs -join ' ')"
    $result = & $tc.CMake @cmakeArgs 2>&1
    if ($LASTEXITCODE -ne 0) {
        Write-Fail "CMake 配置失败"
        Write-Host $result -ForegroundColor Red
        throw "CMake 配置失败"
    }
    Write-Success "CMake 配置完成"

    # CMake 构建
    Write-Info "正在编译..."
    $buildArgs = if ($gen -eq "Ninja") {
        @("--build", $BuildDir, "-j", "0")
    } else {
        @("--build", $BuildDir, "--config", $Config, "--", "/v:m", "/nologo")
    }

    $buildResult = & $tc.CMake @buildArgs 2>&1
    if ($LASTEXITCODE -ne 0) {
        Write-Fail "CMake 构建失败"
        Write-Host $buildResult -ForegroundColor Red
        throw "CMake 构建失败"
    }
    Write-Success "CMake 构建完成"

    # 查找 exe
    if ($gen -eq "Ninja") {
        $outExe = "$BuildDir\CloudSeamanor_game.exe"
    } else {
        $outExe = "$BuildDir\CloudSeamanor_game\$Config\CloudSeamanor_game.exe"
    }

    if (Test-Path $outExe) {
        Write-Success "可执行文件: $outExe"
        return $outExe
    }

    # 尝试其他位置
    Get-ChildItem $BuildDir -Recurse -Filter "CloudSeamanor_game.exe" -ErrorAction SilentlyContinue |
        Select-Object -First 1 -ExpandProperty FullName
}

# ============================================================================
# 运行测试
# ============================================================================
function Invoke-Tests {
    Write-Step "运行单元测试"

    $testExe = "$BuildDir\CloudSeamanor_tests.exe"
    if (-not (Test-Path $testExe)) {
        Write-Warn "测试可执行文件未找到: $testExe"
        Write-Info "请在 CMake 配置时设置 -DBUILD_TESTING=ON"
        return
    }

    Write-Info "测试可执行文件: $testExe"
    & $testExe
    if ($LASTEXITCODE -ne 0) {
        Write-Fail "测试失败 (exit code: $LASTEXITCODE)"
    } else {
        Write-Success "所有测试通过"
    }
}

# ============================================================================
# 清理
# ============================================================================
function Clear-Build {
    Write-Step "清理构建产物"

    $dirs = @("build", "bin", ".vs", "out")
    $patterns = @("*.obj", "*.pch", "*.ilk", "*.idb", "*.pdb", "*.tlog", "*.lastbuildstate",
                  "CMakeCache.txt", "CMakeFiles", "compile_commands.json",
                  ".ninja_deps", ".ninja_log", "build.ninja", "rules.ninja",
                  "CTestTestfile.cmake", "cmake_install.cmake", "install_manifest.txt")

    foreach ($dir in $dirs) {
        $path = Join-Path $EngineRoot $dir
        if (Test-Path $path) {
            Write-Info "删除目录: $dir"
            Remove-Item $path -Recurse -Force -ErrorAction SilentlyContinue
        }
    }

    Write-Success "清理完成"
}

# ============================================================================
# 启动游戏
# ============================================================================
function Start-Game {
    param([string]$ExePath)

    $exe = $ExePath
    if (-not $exe) {
        $candidates = @(
            "$BuildDir\CloudSeamanor_game\Debug\CloudSeamanor_game.exe",
            "$BuildDir\CloudSeamanor_game\Release\CloudSeamanor_game.exe",
            "$BuildDir\CloudSeamanor_game.exe",
            "$BuildDir\Debug\CloudSeamanor_game.exe"
        )
        foreach ($c in $candidates) {
            if (Test-Path $c) {
                $exe = $c
                break
            }
        }
    }

    if (-not $exe -or -not (Test-Path $exe)) {
        Write-Warn "游戏可执行文件未找到。请先运行 Build。"
        return
    }

    Write-Step "启动游戏: $exe"

    # 切换到 exe 所在目录（让 assets 相对路径可用）
    $exeDir = Split-Path $exe -Parent
    Push-Location $exeDir
    try {
        Start-Process $exe -WorkingDirectory $exeDir
    }
    finally {
        Pop-Location
    }

    Write-Info "游戏已在后台启动"
}

# ============================================================================
# 帮助
# ============================================================================
function Show-Help {
    Write-Host @"

  云海山庄 — 一键构建脚本

  用法: .\build-game.ps1 <操作> [选项]

  操作:
    Build     构建游戏（Debug，默认）
    Release   构建游戏（Release）
    Rebuild   清理后重新构建
    Test      构建并运行测试
    Clean     清理所有构建产物
    Run       直接运行游戏（不构建）
    Status    显示工具链状态
    Help      显示本帮助

  选项:
    -Configuration <Debug|Release>  选择构建配置（默认: Debug）
    -NoLaunch                        构建后不自动启动游戏

  示例:
    .\build-game.ps1              # Debug 构建 + 自动启动
    .\build-game.ps1 Release      # Release 构建
    .\build-game.ps1 Test         # 构建并运行测试
    .\build-game.ps1 Rebuild      # 完整重新构建
    .\build-game.ps1 Clean        # 清理
    .\build-game.ps1 Status       # 查看工具链

  推荐工作流:
    1. 首次运行: .\build-game.ps1 Status   # 确认工具链正常
    2. 日常开发: .\build-game.ps1          # 一键构建并启动游戏
    3. 提测前:   .\build-game.ps1 Release  # Release 构建

"@
}

# ============================================================================
# 主入口
# ============================================================================
Write-Host ""
Write-Host "  ===========================================" -ForegroundColor Gray
Write-Host "  云海山庄 Cloud Sea Manor — 构建脚本" -ForegroundColor White
Write-Host "  ===========================================" -ForegroundColor Gray
Write-Host ""

switch ($Action) {
    "Help"    { Show-Help; exit 0 }
    "Status"  { Show-Status; exit 0 }
    "Clean"   { Clear-Build; exit 0 }
    "Run"     { Start-Game; exit 0 }

    "Build" {
        Write-Info "模式: Debug"
        try {
            $exe = Build-CMake -Config "Debug"
            if (-not $NoLaunch) {
                Write-Host ""
                Start-Game -ExePath $exe
            }
        }
        catch {
            Write-Fail $_.Exception.Message
            exit 1
        }
    }

    "Release" {
        Write-Info "模式: Release"
        try {
            $exe = Build-CMake -Config "Release"
            if (-not $NoLaunch) {
                Write-Host ""
                Start-Game -ExePath $exe
            }
        }
        catch {
            Write-Fail $_.Exception.Message
            exit 1
        }
    }

    "Rebuild" {
        Write-Info "模式: Rebuild ($Configuration)"
        try {
            Clear-Build
            Write-Host ""
            $exe = Build-CMake -Config $Configuration
            if (-not $NoLaunch) {
                Write-Host ""
                Start-Game -ExePath $exe
            }
        }
        catch {
            Write-Fail $_.Exception.Message
            exit 1
        }
    }

    "Test" {
        Write-Info "模式: Build + Test"
        try {
            $env:SFML_ROOT = $SFMLRoot
            $tc = Get-Toolchain
            if (-not $tc.CMake) {
                throw "CMake 未找到"
            }

            # 配置测试
            $testBuildDir = "$EngineRoot\build-test"
            if (Test-Path $testBuildDir) {
                Remove-Item $testBuildDir -Recurse -Force -ErrorAction SilentlyContinue
            }

            & $tc.CMake -S $EngineRoot -B $testBuildDir `
                -G "Visual Studio 17 2022" `
                -DCMAKE_BUILD_TYPE=Debug `
                -DBUILD_TESTING=ON `
                -DCLOUDSEAMANOR_ENABLE_WARNINGS=ON

            if ($LASTEXITCODE -ne 0) { throw "CMake 配置失败" }
            Write-Success "配置完成"

            & $tc.CMake --build $testBuildDir --config Debug --target CloudSeamanor_tests
            if ($LASTEXITCODE -ne 0) { throw "测试构建失败" }
            Write-Success "测试构建完成"

            Write-Host ""
            & "$testBuildDir\Debug\CloudSeamanor_tests.exe"
        }
        catch {
            Write-Fail $_.Exception.Message
            exit 1
        }
    }
}

Write-Host ""
Write-Host "  完成。" -ForegroundColor Gray

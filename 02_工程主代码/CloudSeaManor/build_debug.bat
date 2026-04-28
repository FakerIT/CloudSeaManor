@echo off
echo ========================================
echo Cloud Sea Manor Build Script
echo ========================================
echo.

echo Setting up MSVC environment...
call "D:\soft_ware\VisualStudio\VisualStudio_soft\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul 2>&1

if errorlevel 1 (
    echo Failed to set up MSVC environment!
    pause
    exit /b 1
)

echo Environment initialized.
echo.
echo Compiler: %CXX%
echo.
echo ========================================
echo.
echo Attempting CMake configuration...
echo Command: cmake -S . -B build -G "Unix Makefiles"
echo.

cmake -S . -B build -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug -DSFML_ROOT="D:\CloudSeaManor\03_参考与第三方资源\01_SFML引擎\SFML-3.0.2" 2>&1

set CMAKE_EXIT=%ERRORLEVEL%
echo.
echo CMake exit code: %CMAKE_EXIT%
echo.

if %CMAKE_EXIT% EQU 0 (
    echo.
    echo ========================================
    echo CMake configuration succeeded!
    echo Now building...
    echo ========================================
    cmake --build build 2>&1
) else (
    echo.
    echo ========================================
    echo CMake configuration FAILED!
    echo ========================================
)

echo.
echo Done. Press any key to exit...
pause >nul

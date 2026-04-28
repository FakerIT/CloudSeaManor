@echo off
echo ========================================
echo Setting up MSVC environment...
call "D:\soft_ware\VisualStudio\VisualStudio_soft\VC\Auxiliary\Build\vcvarsall.bat" x64
echo ========================================
echo.
echo Checking environment...
echo CXX = %CXX%
echo.
echo Testing cmake directly...
where cmake
cmake --version
echo.
echo ========================================
echo Running cmake...
cmake -S . -B build -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug 2>&1
echo.
echo CMake exit code: %ERRORLEVEL%
pause

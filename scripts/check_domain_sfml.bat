@echo off
REM =============================================================================
REM check_domain_sfml.bat - 检查 domain 层是否违规使用 SFML (Windows)
REM =============================================================================

echo ========================================
echo   Domain 层 SFML 依赖检查
echo ========================================
echo.

set PROJECT_ROOT=%~dp0..
set DOMAIN_DIR=%PROJECT_ROOT%\02_工程主代码\CloudSeaManor\src\domain

if not exist "%DOMAIN_DIR%" (
    echo [警告] domain 目录不存在
    exit /b 0
)

echo 检查目录: %DOMAIN_DIR%
echo.

set VIOLATIONS=0

REM 检查 SFML 命名空间使用
echo [检查 1] SFML 命名空间...
findstr /S /R /C:"sf::" "%DOMAIN_DIR%\*.cpp" "%DOMAIN_DIR%\*.hpp" > temp_sf.txt 2>nul
if %ERRORLEVEL%==0 (
    echo [错误] 发现 sf:: 命名空间使用:
    type temp_sf.txt
    set VIOLATIONS=1
)

REM 检查 SFML 头文件包含
echo.
echo [检查 2] SFML 头文件包含...
findstr /S /R /C:"#include.*SFML" "%DOMAIN_DIR%\*.cpp" "%DOMAIN_DIR%\*.hpp" > temp_inc.txt 2>nul
if %ERRORLEVEL%==0 (
    echo [错误] 发现 SFML 头文件包含:
    type temp_inc.txt
    set VIOLATIONS=1
)

REM 清理临时文件
del temp_sf.txt 2>nul
del temp_inc.txt 2>nul

echo.
echo ========================================
if %VIOLATIONS%==1 (
    echo [失败] 检查未通过，发现违规使用
    exit /b 1
) else (
    echo [成功] 检查通过，domain 层无 SFML 违规使用
)
echo ========================================
exit /b 0

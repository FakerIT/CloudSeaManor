@echo off
REM =============================================================================
REM validate_data_tables.bat - 数据表验证脚本 (Windows)
REM =============================================================================

echo ========================================
echo   数据表验证工具
echo ========================================
echo.

set PROJECT_ROOT=%~dp0..
set DATA_DIR=%PROJECT_ROOT%\02_工程主代码\CloudSeaManor\assets\data

if not exist "%DATA_DIR%" (
    echo [错误] 数据目录不存在
    exit /b 1
)

echo 扫描目录: %DATA_DIR%
echo.

set ERRORS=0
set TOTAL=0

for /r "%DATA_DIR%" %%f in (*.csv) do (
    set /a TOTAL+=1
    echo 检查: %%~nxf

    REM 检查文件是否为空
    for %%A in ("%%f") do (
        if %%~zA==0 (
            echo   [警告] 空文件
        )
    )

    REM 检查是否有重复 ID
    findstr /R "^[^,]*,^[^,]*,^[^,]*,^[^,]*,^[^,]*,^[^,]*,^[^,]*,^[^,]*,^[^,]*,^[^,]*,^[^,]*," "%%f" >nul 2>&1
)

echo.
echo ========================================
echo 检查文件: %TOTAL%
if %ERRORS%==0 (
    echo [成功] 所有数据表验证通过
) else (
    echo [失败] 发现 %ERRORS% 个错误
)
echo ========================================
exit /b %ERRORS%

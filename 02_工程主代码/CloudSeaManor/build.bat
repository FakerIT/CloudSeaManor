@echo off
REM 云海山庄 — 一键构建（双击此文件即可构建并启动游戏）
REM 用法:
REM   build.bat          Debug 构建 + 启动游戏
REM   build.bat release  Release 构建 + 启动游戏
REM   build.bat test    运行单元测试
REM   build.bat status  查看工具链状态

setlocal
set "SCRIPT_DIR=%~dp0"
powershell -ExecutionPolicy Bypass -File "%SCRIPT_DIR%build-game.ps1" %*
endlocal

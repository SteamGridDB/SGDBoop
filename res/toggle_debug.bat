@echo off
cd /d "%~dp0"

:: Check if the bat is being run as admin
net session >nul 2>&1
if not %errorlevel% == 0 (
    echo Please run this script as Administrator.
    pause
	exit
)

if "%SGDBOOP_DEBUG%" == "1" (
    setx SGDBOOP_DEBUG 0 /m >nul
    echo Disabled debugging. Run this script again to enable it.
) else (
    setx SGDBOOP_DEBUG 1 /m >nul
    echo Enabled debugging. Run this script again to disable it.
)

pause
@echo off
cd /d "%~dp0"

:: Check if the bat is being run as admin
net session >nul 2>&1
if not %errorlevel% == 0 (
    echo Please run this script as Administrator.
    pause
	exit
)

SGDBoop.exe unregister
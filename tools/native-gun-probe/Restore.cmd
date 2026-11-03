@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0NATIVE-TEST\Manage.ps1" -Action Restore
if errorlevel 1 (pause & exit /b 1)
pause

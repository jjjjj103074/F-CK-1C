@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0AMMO-TEST\Manage-AmmoProbe.ps1" -Action Collect %*
if errorlevel 1 echo Collection failed. Please send the displayed error.
pause

@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0AMMO-TEST\Manage-AmmoProbe.ps1" -Action Install %*
if errorlevel 1 echo Installation failed. Please send the displayed error.
pause

@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0AMMO-TEST\Manage-AmmoProbe.ps1" -Action Remove %*
if errorlevel 1 echo Removal failed. Please send the displayed error.
pause

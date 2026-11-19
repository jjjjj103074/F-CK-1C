@echo off
setlocal
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0COLLECT-RESULTS.ps1" %*
pause

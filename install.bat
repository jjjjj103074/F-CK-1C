@echo off
rem Simple installer: copies current mod folder to Saved Games\DCS\Mods\aircraft\F-CK-1-C
rem Usage: install.bat [SavedGamesRoot]

set MOD_NAME=F-CK-1-C
set SRC=%~dp0
if "%SRC:~-1%"=="\" set SRC=%SRC:~0,-1%

if not "%~1"=="" (
  set TARGET_ROOT=%~1
) else (
  set TARGET_ROOT=%USERPROFILE%\Saved Games\DCS
)

set DST=%TARGET_ROOT%\Mods\aircraft\%MOD_NAME%

if not exist "%TARGET_ROOT%\Mods\aircraft" mkdir "%TARGET_ROOT%\Mods\aircraft"

rem Remove existing installation (if any) to ensure a clean full replace
if exist "%DST%" (
  echo Removing existing installation at "%DST%" ...
  rmdir /S /Q "%DST%"
)

rem Ensure destination folder exists then copy
mkdir "%DST%" >nul 2>&1
rem Use xcopy for simplicity: /E copy subdirectories, /I assume dest is dir, /Y overwrite
xcopy "%SRC%\*" "%DST%\" /E /I /Y >nul

if errorlevel 1 (
  echo Copy failed. Check permissions and target path.
  pause
  exit /b 1
) else (
  echo Installed to "%DST%"
  pause
  exit /b 0
)

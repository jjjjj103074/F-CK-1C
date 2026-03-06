@echo off
rem Simple uninstaller: removes Saved Games\DCS\Mods\aircraft\F-CK-1-C
rem Usage: uninstall.bat [SavedGamesRoot]

set MOD_NAME=F-CK-1-C

if not "%~1"=="" (
  set TARGET_ROOT=%~1
) else (
  set TARGET_ROOT=%USERPROFILE%\Saved Games\DCS
)

set DST=%TARGET_ROOT%\Mods\aircraft\%MOD_NAME%

echo This will remove "%DST%" if it exists.
set /p CONFIRM=Type YES to proceed: 
if /I not "%CONFIRM%"=="YES" (
  echo Aborted.
  pause
  exit /b 1
)

if exist "%DST%" (
  rmdir /S /Q "%DST%"
  if exist "%DST%" (
    echo Failed to remove "%DST%". Try running as Administrator.
    pause
    exit /b 1
  ) else (
    echo Removed "%DST%"
    pause
    exit /b 0
  )
) else (
  echo Not found: "%DST%"
  pause
  exit /b 0
)

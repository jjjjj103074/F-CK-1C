@echo off
rem Simple installer: copies current mod folder to Saved Games\DCS\Mods\aircraft\F-CK-1C
rem Usage: install.bat [SavedGamesRoot]
setlocal

set MOD_NAME=F-CK-1C
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

rem Ensure destination folder exists then copy only DCS runtime files.
mkdir "%DST%" >nul 2>&1

call :CopyRequiredFile "entry.lua" || goto copy_failed
call :CopyRequiredFile "F-CK-1C.lua" || goto copy_failed
call :CopyRequiredFile "comm.lua" || goto copy_failed
call :CopyRequiredFile "Views.lua" || goto copy_failed

call :CopyOptionalFile "F-CK-1C.png" || goto copy_failed

call :CopyRequiredDir "bin" || goto copy_failed
call :CopyRequiredDir "Cockpit" || goto copy_failed
call :CopyRequiredDir "FM" || goto copy_failed
call :CopyRequiredDir "Input" || goto copy_failed
call :CopyRequiredDir "Liveries" || goto copy_failed
call :CopyRequiredDir "Shapes" || goto copy_failed
call :CopyRequiredDir "Sounds" || goto copy_failed
call :CopyRequiredDir "Textures" || goto copy_failed

call :CopyOptionalDir "Missions" || goto copy_failed
call :CopyOptionalDir "Options" || goto copy_failed

echo Installed to "%DST%"
pause
exit /b 0

:copy_failed
echo Copy failed. Check source files, permissions, and target path.
pause
exit /b 1

:CopyRequiredFile
if not exist "%SRC%\%~1" (
  echo Missing required file: %~1
  exit /b 1
)
copy /Y "%SRC%\%~1" "%DST%\" >nul
exit /b %ERRORLEVEL%

:CopyOptionalFile
if not exist "%SRC%\%~1" exit /b 0
copy /Y "%SRC%\%~1" "%DST%\" >nul
exit /b %ERRORLEVEL%

:CopyRequiredDir
if not exist "%SRC%\%~1" (
  echo Missing required directory: %~1
  exit /b 1
)
xcopy "%SRC%\%~1" "%DST%\%~1\" /E /I /Y >nul
exit /b %ERRORLEVEL%

:CopyOptionalDir
if not exist "%SRC%\%~1" exit /b 0
xcopy "%SRC%\%~1" "%DST%\%~1\" /E /I /Y >nul
exit /b %ERRORLEVEL%

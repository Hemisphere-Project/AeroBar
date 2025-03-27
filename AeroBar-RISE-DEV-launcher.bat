@echo off
setlocal enabledelayedexpansion

echo.
echo ===== AeroBar RISE Launcher =====
echo.

echo. WARNING: The MadMapper project is openend in READ/WRITE mode.
echo.       If you save the project, it will overwrite the original file !
echo.

cd /D "%~dp0"

REM Set PM2 environment variables (critical for Windows)
@REM set PM2_HOME=%USERPROFILE%\.pm2
@REM set PATH=%PATH%;%APPDATA%\npm

@REM kill MadMapper if running
taskkill /IM MadMapper.exe /F >nul 2>&1

REM 1. Path validation for critical components
@REM where pm2 >nul 2>&1
@REM if !errorlevel! neq 0 (
@REM     echo PM2 not found in system PATH
@REM     exit /b 1
@REM )

REM 2. PM2 Process Management with proper error handling
@REM call pm2 stop midi2osc >nul 2>&1
@REM call pm2 del midi2osc >nul 2>&1

@REM Reset midi 
@REM powershell.exe -NoProfile -ExecutionPolicy Bypass -File "reset-midi.ps1"

@REM cd midi2osc
@REM call pm2 start midi2osc.js >nul 2>&1
@REM cd ..
@REM if !errorlevel! neq 0 (
@REM     echo PM2 failed to start midi2osc
@REM ) else (
@REM     echo PM2 started midi2osc
@REM     echo.
@REM )


REM 3. Remaining script commands...
timeout 3 >nul
copy /Y "AeroBar-RISE-original.mad" "%USERPROFILE%\Documents\RISE.mad"
echo Copied RISE-original.mad to Documents\RISE.mad
echo.

REM 4. MadMapper launch with explicit path validation
if exist "C:\Program Files\MadMapper 5.6.6\MadMapper.exe" (
    start "" "C:\Program Files\MadMapper 5.6.6\MadMapper.exe" "AeroBar-RISE-original.mad"
    echo MadMapper started with AeroBar-RISE-original.mad
) else (
    echo MadMapper.exe not found at specified path
)

@REM sleep 5
echo.
echo "Have fun ^!"
echo.
timeout 5 >nul


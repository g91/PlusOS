@echo off
REM ==========================================================================
REM  PlusOS build front-end for Windows.
REM  The m68k cross compiler (Retro68) runs inside WSL Ubuntu; this script
REM  forwards to make there, so you can edit and build entirely from Windows.
REM
REM    build              build SDK libraries, kernel, apps
REM    build sdklibs      build only the two SDK libraries (sdks\macplus, sdks\macse)
REM    build sdks         regenerate both SDK folders from the Plus and SE ROMs
REM    build sdktest      run both SDKs' self-tests
REM    build install      build and write boot block + kernel + apps to the disk images
REM    build test         install, then boot the image in Mini vMac
REM    build clean        remove build outputs
REM    build <target>     any other Makefile target
REM ==========================================================================
setlocal
set "ROOT=%~dp0"
set "TARGET=%*"
if "%TARGET%"=="" set "TARGET=all"

if /I "%~1"=="test" (
    call "%~f0" install || exit /b 1
    powershell -NoProfile -ExecutionPolicy Bypass -File "%ROOT%tools\vmac.ps1" start
    exit /b 0
)

if /I "%~1"=="clean" (
    call "%ROOT%clean.bat" %2 %3 %4
    exit /b %ERRORLEVEL%
)

REM Close the emulator first: it locks the disk image.
tasklist /FI "IMAGENAME eq Mini vMac (BlueSCSI patched).exe" 2>nul | find /I "Mini vMac" >nul && (
    echo Closing Mini vMac so the disk image can be written...
    taskkill /IM "Mini vMac (BlueSCSI patched).exe" /F >nul 2>&1
)

REM If an external SD card (F:) is attached, refresh its mount in WSL.
if exist "F:\" (
    if /I "%~1"=="install" wsl -d Ubuntu -u root -- bash /mnt/c/Users/Evil/PlusOS/tools/remount.sh
    if /I "%~1"=="sdks" wsl -d Ubuntu -u root -- bash /mnt/c/Users/Evil/PlusOS/tools/remount.sh
)

wsl -d Ubuntu --cd /mnt/c/Users/Evil/PlusOS -- make %TARGET%
exit /b %ERRORLEVEL%

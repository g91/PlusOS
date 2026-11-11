@echo off
REM ==========================================================================
REM  PlusOS build front-end for Windows — MSYS2 native (no WSL required).
REM
REM    build              build SDK libraries, kernel, apps
REM    build install      build and write boot block + kernel + apps to disk
REM    build test         install, then launch Mini vMac for testing
REM    build floppy       build minimal bootable floppy image
REM    build clean        remove build outputs
REM    build clean-cache  remove hash-cache sidecars only (force full recompile)
REM    build sdklibs      build only the two SDK libraries
REM    build sdks         regenerate both SDK folders from the Plus and SE ROMs
REM    build sdktest      run both SDKs' self-tests
REM    build <target>     any other Makefile target
REM
REM  First run?  Execute:  tools\winbuild\setup.ps1
REM  Or click "Setup Build Environment" in the Disk Studio.
REM ==========================================================================
setlocal EnableDelayedExpansion

set "ROOT=%~dp0"

REM ── Target machine selection (optional first keyword) ─────────────────────
set "MACHINE_FLAG="
if /I "%~1"=="universal" ( set "MACHINE_FLAG=MACHINE=universal" & shift )
if /I "%~1"=="uni"       ( set "MACHINE_FLAG=MACHINE=universal" & shift )
if /I "%~1"=="se"        ( set "MACHINE_FLAG=MACHINE=se"        & shift )
if /I "%~1"=="plus"      ( set "MACHINE_FLAG=MACHINE=plus"      & shift )
if /I "%~1"=="classic"   ( set "MACHINE_FLAG=MACHINE=classic"   & shift )
if /I "%~1"=="lc3"       ( set "MACHINE_FLAG=MACHINE=lc3"       & shift )

REM ── Optional 'netdbg' keyword: compile in the live net debugger auto-start ─
set "NETDBG_FLAG="
if /I "%~1"=="netdbg"    ( set "NETDBG_FLAG=NETDBG=1"           & shift )

set "TARGET=%1 %2 %3 %4 %5 %6 %7 %8 %9"
for /f "tokens=* delims= " %%T in ("%TARGET%") do set "TARGET=%%T"
if "%TARGET%"=="" set "TARGET=all"

REM ── Locate MSYS2 and toolchain via env.txt (written by setup.ps1) ─────────
set "BASH="
set "TOOLCHAIN_DIR="

if exist "%ROOT%tools\winbuild\env.txt" (
    for /f "usebackq tokens=1,* delims==" %%K in ("%ROOT%tools\winbuild\env.txt") do (
        if /I "%%K"=="MSYS2_BASH"     set "BASH=%%L"
        if /I "%%K"=="TOOLCHAIN_DIR"  set "TOOLCHAIN_DIR=%%L"
    )
)

REM ── Fallback: scan well-known MSYS2 locations ─────────────────────────────
if not defined BASH (
    if exist "%ROOT%tools\winbuild\msys2\usr\bin\bash.exe" (
        set "BASH=%ROOT%tools\winbuild\msys2\usr\bin\bash.exe"
    ) else if exist "C:\msys64\usr\bin\bash.exe" (
        set "BASH=C:\msys64\usr\bin\bash.exe"
    ) else if exist "C:\msys2\usr\bin\bash.exe" (
        set "BASH=C:\msys2\usr\bin\bash.exe"
    )
)

if not defined BASH (
    echo.
    echo  ERROR: MSYS2 build environment not found.
    echo.
    echo  To set up the portable Windows build environment, run ONE of:
    echo    powershell -ExecutionPolicy Bypass -File "%ROOT%tools\winbuild\setup.ps1"
    echo.
    echo  Or open PlusOSDiskStudio.exe and click "Setup Build Environment".
    echo.
    exit /b 1
)

REM ── Convert Windows repo path to MSYS2 path: C:\foo\bar -> /c/foo/bar ─────
for /f "delims=" %%M in ('powershell -NoProfile -Command ^
    "$p='%ROOT:~0,-1%'; '/'+$p[0].ToString().ToLower()+(($p.Substring(2)).Replace('\','/'))"') do set "MSYS2_ROOT=%%M"

if not defined MSYS2_ROOT (
    echo ERROR: Failed to convert repo path to MSYS2 format.
    exit /b 1
)

REM ── Build TOOLCHAIN= override if local Retro68 was built ─────────────────
set "TOOLCHAIN_FLAG="
if defined TOOLCHAIN_DIR (
    if exist "%TOOLCHAIN_DIR%\bin\m68k-apple-macos-gcc.exe" (
        for /f "delims=" %%T in ('powershell -NoProfile -Command ^
            "$p='%TOOLCHAIN_DIR%'; '/'+$p[0].ToString().ToLower()+(($p.Substring(2)).Replace('\','/'))"') do set "MSYS2_TC=%%T"
        if defined MSYS2_TC set "TOOLCHAIN_FLAG=TOOLCHAIN=!MSYS2_TC!"
    )
)

REM ── Close Mini vMac if open (it locks the disk image) ─────────────────────
tasklist /FI "IMAGENAME eq Mini vMac (BlueSCSI patched).exe" 2>nul | find /I "Mini vMac" >nul && (
    echo Closing Mini vMac so the disk image can be written...
    taskkill /IM "Mini vMac (BlueSCSI patched).exe" /F >nul 2>&1
)
tasklist /FI "IMAGENAME eq Mini vMac.exe" 2>nul | find /I "Mini vMac" >nul && (
    taskkill /IM "Mini vMac.exe" /F >nul 2>&1
)

REM ── Handle 'test': build install, then launch emulator ───────────────────
if /I "%TARGET%"=="test" (
    call "%~f0" %MACHINE_FLAG% install || exit /b 1
    powershell -NoProfile -ExecutionPolicy Bypass -File "%ROOT%tools\vmac.ps1" start
    exit /b 0
)

REM ── Handle 'clean' via PowerShell clean script ───────────────────────────
if /I "%TARGET%"=="clean" (
    if exist "%ROOT%clean.ps1" (
        powershell -NoProfile -ExecutionPolicy Bypass -File "%ROOT%clean.ps1" -All
    ) else (
        REM Fallback: remove build/ directly
        if exist "%ROOT%build\" rd /s /q "%ROOT%build"
        echo Build directory cleaned.
    )
    exit /b 0
)

REM ── Remount BlueSCSI SD card in MSYS2 (if attached) ───────────────────────
REM  MSYS2 mounts Windows drives as /c/, /d/, /f/, etc. automatically —
REM  no manual remount needed unlike WSL drvfs.

REM ── Run make inside MSYS2 ────────────────────────────────────────────────
if defined MACHINE_FLAG echo Building for %MACHINE_FLAG%
if defined NETDBG_FLAG echo Net debugger: auto-start ENABLED (NETDBG=1)
if not defined NETDBG_FLAG echo Net debugger: auto-start disabled (default; pass 'netdbg' to enable)
if defined TOOLCHAIN_FLAG echo Toolchain: %TOOLCHAIN_DIR%

set "MSYSTEM=UCRT64"
"%BASH%" --login -c "cd '%MSYS2_ROOT%' && make %MACHINE_FLAG% %NETDBG_FLAG% %TOOLCHAIN_FLAG% %TARGET%"
exit /b %ERRORLEVEL%

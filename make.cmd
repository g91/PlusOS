@echo off
REM ============================================================================
REM  make.cmd - build ALL of PlusOS natively on Windows, no WSL / Linux / GNU.
REM
REM    make.cmd                 build SDK libs, kernel, apps  (MACHINE=se)
REM    make.cmd image           ... and build the bootable hard drive image
REM    make.cmd plus image      target the Macintosh Plus
REM    make.cmd clean
REM    make.cmd <target> [VAR=value ...]   any Makefile.p68 target
REM
REM  First run builds the toolchain (toolchain\bin\*.exe) with MSVC if needed.
REM ============================================================================
setlocal EnableDelayedExpansion
set "ROOT=%~dp0"
cd /d "%ROOT%"

REM ---- optional leading machine keyword, then forward the rest verbatim ------
REM %* keeps VAR=value overrides intact (a per-arg loop would let cmd turn '=' into a space).
set "ALL=%*"
set "MACHINEARG="
if /I "%~1"=="se"        ( set "MACHINEARG=MACHINE=se "        & call :drop1 )
if /I "%~1"=="plus"      ( set "MACHINEARG=MACHINE=plus "      & call :drop1 )
if /I "%~1"=="classic"   ( set "MACHINEARG=MACHINE=classic "   & call :drop1 )
if /I "%~1"=="lc3"       ( set "MACHINEARG=MACHINE=lc3 "       & call :drop1 )
if /I "%~1"=="universal" ( set "MACHINEARG=MACHINE=universal " & call :drop1 )

REM ---- ensure the toolchain exists -------------------------------------------
if not exist "%ROOT%toolchain\bin\p68-cc.exe"  call "%ROOT%toolchain\build.cmd" || exit /b 1
if not exist "%ROOT%toolchain\bin\p68-make.exe" call "%ROOT%toolchain\build.cmd" || exit /b 1

"%ROOT%toolchain\bin\p68-make.exe" -f Makefile.p68 %MACHINEARG%%ALL%
exit /b %ERRORLEVEL%

REM strip the first whitespace-delimited token (the machine keyword) from %ALL%
:drop1
for /f "tokens=1,*" %%a in ("%ALL%") do set "ALL=%%b"
goto :eof

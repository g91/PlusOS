@echo off
REM ==========================================================================
REM  PlusOS Cleanup Script for Windows.
REM  Deletes all compiler intermediate files, object files (*.obj, *.o),
REM  compiled resource files (*.res), debug symbols (*.pdb, *.idb, *.ilk),
REM  dependency lists (*.d), and cache unnecessary for recompiling.
REM
REM  Usage:
REM    clean                  Clean object and intermediate files
REM    clean -a, --all        Full clean (also wipes build/ outputs via make clean)
REM    clean -r, --restore    Restore tracked build outputs in build/ to clean git state
REM    clean -u, --untrack    Untrack any accidentally committed .obj/.res from git
REM    clean -n, --dry-run    Preview files that would be deleted
REM    clean -h, --help       Show help
REM ==========================================================================
setlocal
set "ROOT=%~dp0"

REM Parse arguments
set "ARGS="
:parse_args
if "%~1"=="" goto run
if /I "%~1"=="-a" (
    set "ARGS=%ARGS% -All"
) else if /I "%~1"=="--all" (
    set "ARGS=%ARGS% -All"
) else if /I "%~1"=="-r" (
    set "ARGS=%ARGS% -RestoreTracked"
) else if /I "%~1"=="--restore" (
    set "ARGS=%ARGS% -RestoreTracked"
) else if /I "%~1"=="-u" (
    set "ARGS=%ARGS% -UntrackObjects"
) else if /I "%~1"=="--untrack" (
    set "ARGS=%ARGS% -UntrackObjects"
) else if /I "%~1"=="-n" (
    set "ARGS=%ARGS% -DryRun"
) else if /I "%~1"=="--dry-run" (
    set "ARGS=%ARGS% -DryRun"
) else if /I "%~1"=="-h" (
    set "ARGS=%ARGS% -Help"
) else if /I "%~1"=="--help" (
    set "ARGS=%ARGS% -Help"
) else (
    set "ARGS=%ARGS% %1"
)
shift
goto parse_args

:run
powershell -NoProfile -ExecutionPolicy Bypass -File "%ROOT%clean.ps1" %ARGS%
exit /b %ERRORLEVEL%

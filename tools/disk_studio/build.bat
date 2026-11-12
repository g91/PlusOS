@echo off
setlocal EnableDelayedExpansion
cd /d "%~dp0"

echo =========================================================================
echo  Building PlusOS Disk Studio (Native C++ Win32 GUI)
echo =========================================================================

REM ── Locate and initialise Visual Studio C++ (vcvars64.bat) ────────────────
where cl >nul 2>nul && goto :have_cl
set "VCVARS="
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" set "VSWHERE=%ProgramFiles%\Microsoft Visual Studio\Installer\vswhere.exe"
if exist "%VSWHERE%" (
    for /f "usebackq delims=" %%I in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath 2^>nul`) do (
        if exist "%%I\VC\Auxiliary\Build\vcvars64.bat" set "VCVARS=%%I\VC\Auxiliary\Build\vcvars64.bat"
    )
)
if not defined VCVARS (
    for %%V in (18\Community 2022\Community 2022\Professional 2022\Enterprise 2022\BuildTools 2019\Community 2019\BuildTools) do (
        if exist "%ProgramFiles%\Microsoft Visual Studio\%%V\VC\Auxiliary\Build\vcvars64.bat" set "VCVARS=%ProgramFiles%\Microsoft Visual Studio\%%V\VC\Auxiliary\Build\vcvars64.bat"
        if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\%%V\VC\Auxiliary\Build\vcvars64.bat" set "VCVARS=%ProgramFiles(x86)%\Microsoft Visual Studio\%%V\VC\Auxiliary\Build\vcvars64.bat"
    )
)
if not defined VCVARS (
    echo ERROR: Visual Studio C++ tools not found. Install "Desktop development with C++".
    exit /b 1
)
echo Using: %VCVARS%
call "%VCVARS%" >nul
where cl >nul 2>nul || ( echo ERROR: cl.exe not on PATH after vcvars. & exit /b 1 )
:have_cl

REM ── Compile resource file ────────────────────────────────────────────────
rc /nologo /fo resource.res resource.rc 2>nul
if errorlevel 1 echo Warning: resource compiler had warnings ^(continuing^)

REM ── Compile the application ──────────────────────────────────────────────
cl /nologo /O2 /W3 /EHsc /std:c++17 /utf-8 /DUNICODE /D_UNICODE ^
    main.cpp disk_image.cpp hfs_filesystem.cpp ^
    builder_window.cpp explorer_window.cpp ^
    resource.res ^
    /Fe:PlusOSDiskStudio.exe ^
    /link user32.lib gdi32.lib comctl32.lib shell32.lib comdlg32.lib ole32.lib ^
          /SUBSYSTEM:WINDOWS

if errorlevel 1 (
    echo.
    echo BUILD FAILED.
    exit /b 1
)

echo.
echo Build successful: %~dp0PlusOSDiskStudio.exe

REM ── Copy to repo root ────────────────────────────────────────────────────
set "REPO_ROOT=%~dp0..\.."
copy /y PlusOSDiskStudio.exe "%REPO_ROOT%\PlusOSDiskStudio.exe" >nul
if errorlevel 1 (
    echo Warning: could not copy to repo root.
) else (
    echo Deployed: %REPO_ROOT%\PlusOSDiskStudio.exe
)

exit /b 0

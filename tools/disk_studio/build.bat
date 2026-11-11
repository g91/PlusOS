@echo off
setlocal EnableDelayedExpansion
cd /d "%~dp0"

echo =========================================================================
echo  Building PlusOS Disk Studio (Native C++ Win32 GUI)
echo =========================================================================

REM ── Locate vcvars64.bat (try latest VS first via vswhere, then known paths) ─
set "VCVARS="

REM Try vswhere (most reliable)
for %%W in (
    "C:\Program Files\Microsoft Visual Studio\Installer\vswhere.exe"
    "C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe"
) do (
    if not defined VCVARS (
        if exist %%W (
            for /f "usebackq delims=" %%I in (`%%W -latest -property installationPath 2^>nul`) do (
                if exist "%%I\VC\Auxiliary\Build\vcvars64.bat" (
                    set "VCVARS=%%I\VC\Auxiliary\Build\vcvars64.bat"
                )
            )
        )
    )
)

REM Manual fallback scan
if not defined VCVARS (
    for %%V in (
        "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"
        "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
        "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat"
        "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat"
        "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"
        "C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\VC\Auxiliary\Build\vcvars64.bat"
    ) do (
        if not defined VCVARS if exist %%V set "VCVARS=%%V"
    )
)

if not defined VCVARS (
    echo ERROR: Visual Studio not found.  Install VS 2019 or later with the C++ workload.
    exit /b 1
)

echo Using: %VCVARS%
call "%VCVARS%" >nul 2>&1
if errorlevel 1 (
    echo ERROR: Failed to initialise MSVC environment.
    exit /b 1
)

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

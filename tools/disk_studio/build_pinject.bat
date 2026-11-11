@echo off
setlocal EnableDelayedExpansion
cd /d "%~dp0"
echo Building pinject (native console injector)...

set "VCVARS="
for %%W in (
    "C:\Program Files\Microsoft Visual Studio\Installer\vswhere.exe"
    "C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe"
) do (
    if not defined VCVARS if exist %%W (
        for /f "usebackq delims=" %%I in (`%%W -latest -property installationPath 2^>nul`) do (
            if exist "%%I\VC\Auxiliary\Build\vcvars64.bat" set "VCVARS=%%I\VC\Auxiliary\Build\vcvars64.bat"
        )
    )
)
if not defined VCVARS (
    for %%V in (
        "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
        "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat"
        "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat"
    ) do if not defined VCVARS if exist %%V set "VCVARS=%%V"
)
if not defined VCVARS ( echo ERROR: Visual Studio not found. & exit /b 1 )
call "%VCVARS%" >nul 2>&1

cl /nologo /O2 /W3 /EHsc /std:c++17 /utf-8 /DUNICODE /D_UNICODE ^
    pinject.cpp disk_image.cpp hfs_filesystem.cpp ^
    /Fe:pinject.exe /link kernel32.lib

if errorlevel 1 ( echo BUILD FAILED & exit /b 1 )
echo Build successful: %~dp0pinject.exe
set "REPO_ROOT=%~dp0..\.."
copy /y pinject.exe "%REPO_ROOT%\pinject.exe" >nul && echo Deployed: %REPO_ROOT%\pinject.exe
exit /b 0

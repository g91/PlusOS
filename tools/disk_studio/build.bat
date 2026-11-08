@echo off
setlocal
cd /d "%~dp0"

echo =========================================================================
echo  Building PlusOS Disk Studio (Native C++ Win32 GUI)
echo =========================================================================

call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
if errorlevel 1 (
    echo Error: Could not initialize MSVC environment.
    exit /b 1
)

rc /nologo /fo resource.res resource.rc
if errorlevel 1 (
    echo Warning: Resource compilation had warnings.
)

cl /nologo /O2 /W3 /EHsc /std:c++17 /utf-8 /DUNICODE /D_UNICODE main.cpp disk_image.cpp hfs_filesystem.cpp builder_window.cpp explorer_window.cpp resource.res ^
   /Fe:PlusOSDiskStudio.exe ^
   /link user32.lib gdi32.lib comctl32.lib shell32.lib comdlg32.lib ole32.lib /SUBSYSTEM:WINDOWS

if errorlevel 1 (
    echo Build failed.
    exit /b 1
)

echo.
echo Build successful: %~dp0PlusOSDiskStudio.exe
exit /b 0

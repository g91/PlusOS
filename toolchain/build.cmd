@echo off
REM ============================================================================
REM  build.cmd - build the P68 toolchain natively on Windows with MSVC (cl.exe).
REM  No WSL, no Linux, no GNU tools. Produces toolchain\bin\*.exe:
REM     p68-cc.exe   C/C++ compiler (Motorola 680x0)   + built-in assembler
REM     p68-as.exe   standalone assembler
REM     p68-ld.exe   linker / .pex builder
REM     p68-ar.exe   archiver
REM     p68-make.exe make-compatible build tool
REM
REM  Run from any prompt; it locates and initialises Visual Studio itself.
REM ============================================================================
setlocal EnableDelayedExpansion
set "ROOT=%~dp0"
cd /d "%ROOT%"

REM ---- find and initialise Visual Studio (vcvars64.bat) ---------------------
where cl >nul 2>nul && goto :have_cl
set "VCVARS="
if defined VCINSTALLDIR if exist "%VCINSTALLDIR%Auxiliary\Build\vcvars64.bat" set "VCVARS=%VCINSTALLDIR%Auxiliary\Build\vcvars64.bat"
if not defined VCVARS (
  set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
  if exist "!VSWHERE!" (
    for /f "usebackq delims=" %%I in (`"!VSWHERE!" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath 2^>nul`) do set "VSDIR=%%I"
    if exist "!VSDIR!\VC\Auxiliary\Build\vcvars64.bat" set "VCVARS=!VSDIR!\VC\Auxiliary\Build\vcvars64.bat"
  )
)
if not defined VCVARS (
  for %%V in (18\Community 2022\Community 2022\BuildTools 2019\Community 2019\BuildTools 2017\Community 2017\BuildTools) do (
    if exist "%ProgramFiles%\Microsoft Visual Studio\%%V\VC\Auxiliary\Build\vcvars64.bat" set "VCVARS=%ProgramFiles%\Microsoft Visual Studio\%%V\VC\Auxiliary\Build\vcvars64.bat"
    if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\%%V\VC\Auxiliary\Build\vcvars64.bat" set "VCVARS=%ProgramFiles(x86)%\Microsoft Visual Studio\%%V\VC\Auxiliary\Build\vcvars64.bat"
  )
)
if not defined VCVARS (
  echo ERROR: Visual Studio C++ tools not found. Install "Desktop development with C++".
  exit /b 1
)
echo Using !VCVARS!
call "!VCVARS!" >nul
where cl >nul 2>nul || ( echo ERROR: cl.exe still not on PATH after vcvars. & exit /b 1 )
:have_cl

if not exist "%ROOT%bin"  mkdir "%ROOT%bin"
if not exist "%ROOT%obj"  mkdir "%ROOT%obj"

REM /F sets a large stack (the recursive-descent parser and expression evaluator recurse deeply;
REM MSVC defaults to a 1 MB stack where g++ uses 8 MB, which overflows on big inputs).
set "CXXFLAGS=/nologo /std:c++17 /EHsc /O2 /GR- /W2 /F33554432 /wd4244 /wd4267 /wd4018 /wd4146 /wd4996 /D_CRT_SECURE_NO_WARNINGS"

echo Building p68-cc.exe ...
set "CCOBJS="
for %%F in (src\cc\pp.cpp src\cc\parse.cpp src\cc\expr.cpp src\cc\irgen.cpp src\cc\opt.cpp src\cc\m68k.cpp src\cc\main.cpp) do (
  cl %CXXFLAGS% /DP68_HAVE_AS /c "%%F" /Fo"obj\cc_%%~nF.obj" || goto :err
  set "CCOBJS=!CCOBJS! obj\cc_%%~nF.obj"
)
REM the assembler, shared by p68-cc and p68-as (separate obj names to avoid m68k.cpp collision)
cl %CXXFLAGS% /DP68_HAVE_AS /c src\as\assembler.cpp /Fo"obj\as_assembler.obj" || goto :err
cl %CXXFLAGS% /DP68_HAVE_AS /c src\as\m68k.cpp /Fo"obj\as_m68k.obj" || goto :err
cl %CXXFLAGS% /Fe"bin\p68-cc.exe" !CCOBJS! obj\as_assembler.obj obj\as_m68k.obj || goto :err

echo Building p68-as.exe ...
cl %CXXFLAGS% /Fe"bin\p68-as.exe" src\as\main.cpp obj\as_assembler.obj obj\as_m68k.obj || goto :err

echo Building p68-ld.exe ...
cl %CXXFLAGS% /Fe"bin\p68-ld.exe" src\ld\linker.cpp || goto :err

echo Building p68-ar.exe ...
cl %CXXFLAGS% /Fe"bin\p68-ar.exe" src\ld\ar_main.cpp || goto :err

echo Building p68-make.exe ...
cl %CXXFLAGS% /Fe"bin\p68-make.exe" src\make\make.cpp || goto :err

del /q *.obj >nul 2>&1
echo.
echo Done. Tools in %ROOT%bin:
for %%T in (p68-cc p68-as p68-ld p68-ar p68-make) do if exist "%ROOT%bin\%%T.exe" echo    %%T.exe
exit /b 0

:err
echo BUILD FAILED
exit /b 1

@echo off
REM run.cmd - native Windows compiler test runner. Compiles each tests\cc\t*.c at every
REM optimisation level with p68-cc.exe, links with p68-ld, runs on the emulator CPU core.
REM Needs NODE set to a node.exe (>=18); defaults to node on PATH.
setlocal EnableDelayedExpansion
cd /d "%~dp0"
set "TC=..\.."
set "CC=%TC%\bin\p68-cc.exe"
set "LD=%TC%\bin\p68-ld.exe"
set "AR=%TC%\bin\p68-ar.exe"
set "OUT=%TC%\obj\cctest"
if not exist "%OUT%" mkdir "%OUT%"
if not defined NODE set "NODE=node"
set "RUN68K=%TC%\..\emulator\tools\run68k.mjs"

call "%CC%" -m68000 -c crt0.S -o "%OUT%\crt0.o" || exit /b 1
call "%CC%" -m68000 -c "%TC%\rt\lmath.S" -o "%OUT%\lmath.o" || exit /b 1
call "%CC%" -m68000 -O2 -c "%TC%\rt\int64.c" -o "%OUT%\int64.o" || exit /b 1
call "%CC%" -m68000 -O2 -c "%TC%\rt\softfloat.c" -o "%OUT%\softfloat.o" || exit /b 1
call "%AR%" rcs "%OUT%\libp68rt.a" "%OUT%\lmath.o" "%OUT%\int64.o" "%OUT%\softfloat.o" || exit /b 1

set /a pass=0
set /a fail=0
for %%T in (t*.c) do (
  for %%O in (-O0 -O1 -O2 -Os) do call :one "%%~nT" "%%T" %%O
)
echo passed !pass!, failed !fail!
if !fail! gtr 0 exit /b 1
exit /b 0

:one
set "N=%~1"
set "SRC=%~2"
set "O=%3"
call "%CC%" -m68000 %O% -I. -c "%SRC%" -o "%OUT%\%N%.o" 2>"%OUT%\%N%.err"
if errorlevel 1 ( set /a fail+=1 & echo FAIL-compile %N% %O% & goto :eof )
call "%LD%" --oformat binary -Ttext 0x10000 -e _start -o "%OUT%\%N%.bin" "%OUT%\crt0.o" "%OUT%\%N%.o" "%OUT%\libp68rt.a" 2>>"%OUT%\%N%.err"
if errorlevel 1 ( set /a fail+=1 & echo FAIL-link %N% %O% & goto :eof )
call "%NODE%" "%RUN68K%" --max-insns 200000000 "%OUT%\%N%.bin" >"%OUT%\%N%.out" 2>&1
if errorlevel 1 ( set /a fail+=1 & echo FAIL %N% %O% & type "%OUT%\%N%.out" & goto :eof )
set /a pass+=1
goto :eof

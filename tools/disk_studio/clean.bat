@echo off
REM Clean Disk Studio MSVC compiler artifacts
call "%~dp0..\..\clean.bat" %*
exit /b %ERRORLEVEL%

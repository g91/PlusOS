@echo off
REM One-click launcher for the PlusOS OTA server on Windows.
REM   run.cmd            - start the server (127.0.0.1:8799)
REM   run.cmd shell      - start with the control-panel shell enabled
setlocal
cd /d "%~dp0"

if not exist ".venv\" (
  echo [setup] creating virtual env + installing deps...
  python -m venv .venv
  call .venv\Scripts\activate.bat
  python -m pip install --upgrade pip >nul
  python -m pip install -r requirements.txt
) else (
  call .venv\Scripts\activate.bat
)

if /i "%1"=="shell" set OTA_ALLOW_SHELL=1

echo [run] PlusOS OTA server starting on http://127.0.0.1:8799
echo       control panel: http://127.0.0.1:8799/admin
python server.py
endlocal

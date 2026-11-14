@echo off
REM Launch the IKANET server. Open http://localhost:8787 afterward.
cd /d "%~dp0server"
python -m pip install -q -r requirements.txt
python -m uvicorn main:app --host 0.0.0.0 --port 8787

@echo off
setlocal
cd /d "%~dp0"

echo =========================================================================
echo  Starting PlusOS Standalone Package Repository Server
echo  Dual Mode: HTTP (Port 8088 for 68k Macs) ^& HTTPS (Port 8443 for Browsers)
echo =========================================================================

python server.py --port 8088 --ssl-port 8443 %*
pause

@echo off
REM ============================================================================
REM  PlusOS OTA — install the server + Cloudflare tunnel as always-on services.
REM  RUN THIS AS ADMINISTRATOR, after setup_tunnel.cmd has succeeded.
REM
REM  It installs two auto-start pieces that survive reboots:
REM    1. cloudflared  — as a real Windows service (cloudflared's own installer),
REM                      using THIS config.yml so it serves ota.d0a.net.
REM    2. PlusOS OTA   — the FastAPI server, as a Scheduled Task that runs at
REM                      startup (and restarts on failure) under this account.
REM
REM  To enable the admin-panel shell, set OTA_ALLOW_SHELL=1 in the task's
REM  environment (see the note at the bottom) — it is OFF by default.
REM ============================================================================
setlocal EnableDelayedExpansion
cd /d "%~dp0"

net session >nul 2>nul
if errorlevel 1 ( echo ERROR: run this script as Administrator. & exit /b 1 )

set "CFG=%~dp0config.yml"
set "OTADIR=%~dp0.."
for %%I in ("%OTADIR%") do set "OTADIR=%%~fI"

echo(
echo === 1/2: install cloudflared as a Windows service ===
REM cloudflared reads a service config; the simplest robust form is to install
REM the service pointed at our config file. Remove any prior install first.
cloudflared service uninstall >nul 2>nul
cloudflared --config "%CFG%" service install
if errorlevel 1 (
    echo WARNING: 'service install' failed. You can still run the tunnel by hand:
    echo    cloudflared tunnel --config "%CFG%" run plusos-ota
) else (
    sc config cloudflared start= auto >nul 2>nul
    net start cloudflared >nul 2>nul
    echo cloudflared service installed and started.
)

echo(
echo === 2/2: install the PlusOS OTA server as a startup Scheduled Task ===
REM Use run.cmd so the venv is created/activated. SYSTEM-independent: runs as the
REM current user so it can read .publish_key. /RL HIGHEST lets it bind the port.
set "RUNCMD=%OTADIR%\run.cmd"
if not exist "%RUNCMD%" ( echo ERROR: %RUNCMD% missing. & exit /b 1 )

schtasks /Query /TN "PlusOS OTA Server" >nul 2>nul && schtasks /Delete /TN "PlusOS OTA Server" /F >nul 2>nul

schtasks /Create /TN "PlusOS OTA Server" /SC ONSTART /RL HIGHEST /F ^
    /TR "cmd /c cd /d \"%OTADIR%\" && call run.cmd" ^
    /RU "%USERNAME%"
if errorlevel 1 (
    echo WARNING: could not create the scheduled task. Start the server by hand with run.cmd.
) else (
    echo Scheduled task 'PlusOS OTA Server' created (runs at startup).
    echo Starting it now...
    schtasks /Run /TN "PlusOS OTA Server" >nul 2>nul
)

echo(
echo ============================================================================
echo  Done. ota.d0a.net should come up within a minute of boot.
echo(
echo  Check it:
echo    - local  : http://127.0.0.1:8799/healthz   (should print 'ok')
echo    - public : https://ota.d0a.net/healthz
echo(
echo  Manage:
echo    cloudflared:  sc stop cloudflared  /  sc start cloudflared
echo    OTA server :  schtasks /End  /TN "PlusOS OTA Server"
echo                  schtasks /Run  /TN "PlusOS OTA Server"
echo(
echo  To enable the control-panel shell, edit run.cmd (set OTA_ALLOW_SHELL=1)
echo  or set it as a system environment variable, then restart the task.
echo ============================================================================
endlocal

@echo off
REM ============================================================================
REM  PlusOS OTA — Cloudflare Tunnel setup for ota.d0a.net
REM
REM  Run this ONCE, in a normal (non-admin) terminal, with d0a.net already a
REM  zone in your Cloudflare account. It will:
REM    1. install cloudflared (via winget) if it is missing
REM    2. log you in to Cloudflare (browser opens; pick the d0a.net zone)
REM    3. create a tunnel named 'plusos-ota'
REM    4. write the real credentials path into config.yml
REM    5. create the ota.d0a.net DNS record pointing at the tunnel
REM
REM  After it finishes, install it as an always-on service with
REM  install_service.cmd (run that one as Administrator).
REM ============================================================================
setlocal EnableDelayedExpansion
cd /d "%~dp0"

set TUNNEL=plusos-ota
set HOSTNAME=ota.d0a.net

echo(
echo === Step 1: ensure cloudflared is installed ===
where cloudflared >nul 2>nul
if errorlevel 1 (
    echo cloudflared not found - installing with winget...
    winget install --id Cloudflare.cloudflared -e --accept-source-agreements --accept-package-agreements
    REM winget may add it to PATH only for new shells; locate it for this session.
    where cloudflared >nul 2>nul || set "PATH=%PATH%;%LOCALAPPDATA%\Microsoft\WinGet\Links;%ProgramFiles%\cloudflared"
)
where cloudflared >nul 2>nul || ( echo ERROR: cloudflared still not on PATH. Open a NEW terminal and re-run. & exit /b 1 )
cloudflared --version

echo(
echo === Step 2: log in to Cloudflare (a browser window will open) ===
echo Pick the d0a.net zone when asked, then come back here.
cloudflared tunnel login
if errorlevel 1 ( echo ERROR: login failed. & exit /b 1 )

echo(
echo === Step 3: create the tunnel '%TUNNEL%' ===
REM If it already exists this prints an error we can ignore.
cloudflared tunnel create %TUNNEL%

echo(
echo === Step 4: find the credentials file and write it into config.yml ===
set "CREDS="
for /f "delims=" %%F in ('dir /b /s "%USERPROFILE%\.cloudflared\*.json" 2^>nul') do (
    REM the tunnel's own creds file is named <UUID>.json (not cert.pem); take the newest
    set "CREDS=%%F"
)
if not defined CREDS (
    echo ERROR: could not find a tunnel credentials .json in %USERPROFILE%\.cloudflared
    echo Run 'cloudflared tunnel list' and set credentials-file in config.yml by hand.
    exit /b 1
)
echo Using credentials: !CREDS!
REM Rewrite the credentials-file line in config.yml
powershell -NoProfile -Command ^
  "$p='config.yml'; $c=Get-Content $p -Raw;" ^
  "$c=[regex]::Replace($c,'credentials-file:.*','credentials-file: ' + [regex]::Escape('!CREDS!').Replace('\\','\'));" ^
  "Set-Content $p $c -Encoding UTF8"
echo config.yml updated.

REM Also copy the config to cloudflared's default location, so running it as a
REM service (which ignores a --config given only at install on some versions)
REM still finds the right ingress + credentials.
if not exist "%USERPROFILE%\.cloudflared" mkdir "%USERPROFILE%\.cloudflared"
copy /y config.yml "%USERPROFILE%\.cloudflared\config.yml" >nul
echo Copied config to %USERPROFILE%\.cloudflared\config.yml

echo(
echo === Step 5: route ota.d0a.net to the tunnel (creates the DNS record) ===
cloudflared tunnel route dns %TUNNEL% %HOSTNAME%
if errorlevel 1 (
    echo NOTE: if this says the record exists, that is fine.
)

echo(
echo ============================================================================
echo  Tunnel setup complete.
echo    Hostname : https://%HOSTNAME%
echo    Tunnel   : %TUNNEL%
echo    Config   : %~dp0config.yml
echo(
echo  Next: start the OTA server, then the tunnel.
echo    - quick test (two terminals):
echo         otaserver\run.cmd
echo         cloudflared tunnel --config "%~dp0config.yml" run %TUNNEL%
echo    - always-on: run install_service.cmd as Administrator
echo ============================================================================
endlocal

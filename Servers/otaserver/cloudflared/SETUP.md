# Getting ota.d0a.net live (Cloudflare Tunnel)

This connects the local PlusOS OTA server to the internet at **https://ota.d0a.net**
through a Cloudflare Tunnel. Nothing on your router needs port-forwarding —
`cloudflared` dials *out* to Cloudflare and Cloudflare routes `ota.d0a.net` back
down the tunnel to your machine. WebSockets (the live update feed and the native
bridge are proxied over it) work out of the box.

Prerequisite: **d0a.net is already a zone in your Cloudflare dashboard** (its
nameservers point to Cloudflare). If you can see d0a.net at dash.cloudflare.com,
you're set.

## The short version (3 steps)

```cmd
cd Servers\otaserver\cloudflared

REM 1. one-time: install cloudflared, log in, create the tunnel + DNS record
setup_tunnel.cmd

REM 2. one-time, AS ADMINISTRATOR: make the server + tunnel always-on
install_service.cmd

REM 3. check it
curl https://ota.d0a.net/healthz        REM should print: ok
```

That's it. `setup_tunnel.cmd` walks you through the Cloudflare login (a browser
opens — pick **d0a.net**), creates a tunnel named `plusos-ota`, writes the
credentials path into `config.yml`, and creates the `ota.d0a.net` DNS record.
`install_service.cmd` installs `cloudflared` as a Windows service and the OTA
server as a startup Scheduled Task, so both come back after a reboot.

## What each piece is

| Piece | What it does | Port |
|-------|--------------|------|
| OTA server (`run.cmd`) | FastAPI: catalog, publish, admin panel, chat | 127.0.0.1:**8799** |
| native bridge (in-process) | line protocol for the 68k PlusOS app | 0.0.0.0:**8990** |
| cloudflared | exposes 8799 as https://ota.d0a.net | — (outbound) |

The tunnel only publishes the **HTTP** service (8799). The native-client bridge
(8990) is a raw-TCP protocol that Cloudflare's HTTP tunnel can't carry, so the
68k app reaches it on your **LAN / real hardware** directly (e.g. `plusupdate
<your-LAN-ip> 8990`), which is where PlusNET works anyway. The web client, the
host-side Updater, and chat all go through https://ota.d0a.net.

## Testing before you commit to services

Two terminals, no admin, no reboot needed:

```cmd
REM terminal 1 — the server
cd Servers\otaserver
run.cmd
REM note the admin password it prints on first run (or set OTA_ADMIN_PASSWORD)

REM terminal 2 — the tunnel
cloudflared tunnel --config "%USERPROFILE%\.cloudflared\config.yml" run plusos-ota
```

Then open **https://ota.d0a.net/** (client) and **https://ota.d0a.net/admin**
(control panel). `Ctrl-C` in each terminal stops them.

## Publishing through the public URL

Once it's up, the publisher and Disk Studio's **Push Update** button target it:

```cmd
make.cmd ota OTA_SERVER=https://ota.d0a.net OTA_CHANNEL=stable OTA_TITLE="First public build"
```

`OTA_SERVER` already defaults to `https://ota.d0a.net`, so you can usually omit it.

## If d0a.net already has a tunnel (for Servers\web)

You don't need a second tunnel. Add another ingress rule to that tunnel's config:

```yaml
ingress:
  - hostname: ota.d0a.net
    service: http://127.0.0.1:8799
  - hostname: www.d0a.net          # your existing site
    service: http://127.0.0.1:8080
  - service: http_status:404
```

and `cloudflared tunnel route dns <that-tunnel> ota.d0a.net`. Skip
`setup_tunnel.cmd` in that case.

## Troubleshooting

- **`curl https://ota.d0a.net/healthz` hangs or 502** → the tunnel is up but the
  server isn't. Check `http://127.0.0.1:8799/healthz` locally; start `run.cmd`.
- **DNS doesn't resolve** → the `route dns` step didn't run; re-run
  `cloudflared tunnel route dns plusos-ota ota.d0a.net`, or add a CNAME
  `ota` → `<TUNNEL-UUID>.cfargotunnel.com` (proxied) in the Cloudflare DNS tab.
- **Service didn't find its config** → copy `config.yml` to
  `%USERPROFILE%\.cloudflared\config.yml` (setup does this) and
  `sc stop cloudflared & sc start cloudflared`.
- **See what the tunnel is doing** → `cloudflared tunnel info plusos-ota` and the
  Windows Event Viewer (service logs), or run it by hand to watch the console.

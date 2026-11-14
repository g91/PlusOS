# IKANET — Deploying behind Cloudflare

Everything (browser client **and** the native 68k client) now rides **one
WebSocket endpoint on one port**, so it goes cleanly through Cloudflare's
standard proxy. No raw-TCP bridge, no Spectrum, no extra ports.

```
 Browser  ──wss://ikanet.d0a.net/ws──┐
                                          ├─ Cloudflare (443, TLS) ─→ origin:8787 (plain http/ws)
 68k Mac  ──ws://ikanet.d0a.net/ws ───┘      (TLS terminated at Cloudflare)
```

## 1. DNS / subdomain

Create ONE record in Cloudflare DNS:

| Type | Name | Content | Proxy |
|------|------|---------|-------|
| A (or CNAME) | `ikanet` | your origin's public IP (or tunnel) | **Proxied** (orange cloud) |

That gives you `ikanet.d0a.net`. Nothing else is needed.

## 2. Ports to forward

Cloudflare's proxy only passes HTTP/HTTPS/WebSocket on a fixed set of ports;
**443 is the one you want**. So:

- **Inbound to forward on your router/firewall: just `443` → your origin.**
  - If you terminate TLS at Cloudflare and run the origin in plain HTTP, forward
    `443 (public) → 8787 (origin)`, or put a reverse proxy on 443 that proxies to
    `127.0.0.1:8787`.
  - You do **not** open 8787 to the world, and you do **not** open 8788 at all
    (the old raw-TCP bridge is retired — Cloudflare can't proxy raw TCP without
    the paid Spectrum product).

Cloudflare proxies WebSocket automatically on 443; no toggle needed on paid or
free plans (WebSockets are on by default).

### Easiest: Cloudflare Tunnel (no port-forwarding at all)

```
cloudflared tunnel create ikanet
cloudflared tunnel route dns ikanet ikanet.d0a.net
# config.yml:
#   ingress:
#     - hostname: ikanet.d0a.net
#       service: http://localhost:8787
#     - service: http_status:404
cloudflared tunnel run ikanet
```
The tunnel dials out to Cloudflare, so you open **no inbound ports**. The origin
stays on plain `http://localhost:8787`.

## 3. Run the origin

```
cd ikanet/server
python -m pip install -r requirements.txt
python main.py                 # binds 0.0.0.0:8787 (http + /ws)
```

Put a reverse proxy (or the tunnel above) in front so the public edge is 443.

### Example: Caddy reverse proxy (auto-HTTPS if you skip Cloudflare TLS)
```
ikanet.d0a.net {
    reverse_proxy 127.0.0.1:8787    # proxies HTTP and WebSocket transparently
}
```
### Example: nginx (must pass the upgrade headers for WS)
```
server {
    listen 443 ssl;
    server_name ikanet.d0a.net;
    location / {
        proxy_pass http://127.0.0.1:8787;
        proxy_http_version 1.1;
        proxy_set_header Upgrade $http_upgrade;
        proxy_set_header Connection "upgrade";
        proxy_set_header Host $host;
    }
}
```

## 4. Point the clients at the domain

- **Browser**: just open `https://ikanet.d0a.net/` — the page builds the
  WebSocket URL from `location.host`, so it uses `wss://ikanet.d0a.net/ws`
  automatically.
- **Native 68k client** (`ikanet.pex`):
  ```
  ikanet ikanet.d0a.net 80 /ws
  ```
  - arg 1: host (its DaynaPORT resolves it via DNS)
  - arg 2: TCP port — use **80** so Cloudflare upgrades HTTP→WS; the edge still
    serves it on 443/TLS to the outside, and the 68k never does TLS.
  - arg 3: path (`/ws`, the default)

  > The 68k Mac speaks **plain ws://** to the edge; Cloudflare/your proxy is what
  > makes it `wss://` to the internet. A 68000 doing TLS itself is impractical,
  > which is why the TLS front door exists.

## Summary — what you actually forward

| You need | Value |
|----------|-------|
| Subdomain | `ikanet.d0a.net` (one A/CNAME, proxied) |
| Public port | **443** only (or 0 ports with a Cloudflare Tunnel) |
| Origin port | 8787 (kept private; never exposed directly) |
| Retired | 8788 raw-TCP bridge — not used, not forwarded |

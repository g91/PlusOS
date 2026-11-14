# IKANET

A System 6/7-era chat client with real accounts, rooms, DMs, and file sharing —
styled pixel-for-pixel after the classic Mac OS look. Client is a standalone web
app; the server is FastAPI + WebSocket + SQLite.

```
ikanet/
  server/     FastAPI + websockets + SQLite backend
  client/     the web client (login + chat window, all mockups)
  data/       ikanet.db + uploads/  (created on first run; gitignored)
```

## Run

```sh
cd ikanet/server
python -m pip install -r requirements.txt
python main.py                 # or: python -m uvicorn main:app --port 8787
```

Then open **http://localhost:8787** in a browser. Hit **JOIN IKANET** to make an
account, or **LOGIN** if you already have one. Open a second browser/incognito
window, join as another user, and chat in `#general` in real time.

## Production (ikanet.d0a.net)

Both clients default to the public subdomain **`ikanet.d0a.net`** over one
WebSocket endpoint on one port (443), so it goes straight through Cloudflare:

- **Browser**: open `https://ikanet.d0a.net/`.
- **Native 68k client**: just run `ikanet` (defaults to `ikanet.d0a.net 80 /ws`),
  or override with `ikanet HOST PORT PATH`.

See [DEPLOY.md](DEPLOY.md) for the Cloudflare DNS record, the single port to
forward (443 → origin 8787) or a zero-port Cloudflare Tunnel, and reverse-proxy
configs.

## Features (from the mockups)

- **Login screen** — User/Pass, JOIN IKANET + LOGIN (mockup #3).
- **Accounts** — open registration, bcrypt-hashed passwords, session tokens.
- **Rooms & DMs** — `#channels` plus private `dm:` channels. Channel menu:
  Go To Channel List / Add Channel / Leave Channel.
- **Live chat** over WebSocket with persisted history (last 60 messages/room).
- **User icons** — 16×16 mono bitmap editor (File ▸ User Icon…), shown next to
  every message, just like the mockups.
- **File & image sharing**
  - **Upload squid** (and File ▸ Upload File): pick a file; it uploads and posts
    *immediately*, independent of whatever is in the typing box.
  - **Download squid** (and File ▸ Download File): click it to enter download-cursor
    mode, then click any file/image to save it. Status strip shows `SAVING, SAVING`.
  - Image chips open a **draggable preview window**; other files open a
    **File Information** window (kind / size / dimensions) with a Download button.
- **Ping meter** — live avg RTT, "Nice Ping! / OK Ping / Slow Ping…" with signal
  bars, recomputed every 10s (top-right, mockup #2).
- **Status strip** — random retro **Quote of the Day**, activity text
  (`SWIMMING, SWIMMING` for text, `INKING` for images), and the fixed `IKANET` mark.
- **Menus** — File / Edit / Channel / Font (Chicago, Times, Arial, Compact) /
  DarkMode⇄LightMode (simple invert), plus **Settings**.
- **Appear As Offline** toggle.

## Bonus: the squid cursor + video settings (as requested)

- Inside the app the mouse is the **IKANET squid cursor**; outside it's normal.
- **Settings ▸ Cursor ▸ System Squid Cursor** turns the squid cursor on for the
  *whole* page/system — the sneak-peek feature.
- **Settings ▸ Video** — Resolution, Color Depth, and a **Video RAM** slider that
  computes the framebuffer size against available VRAM and warns if there isn't
  enough (e.g. 512×342×1-bit vs 8 MB). Depth defaults to 1-bit to match PlusOS gfx.

## Data

SQLite at `ikanet/data/ikanet.db`; uploads under `ikanet/data/uploads/`.
Delete the `data/` folder to reset everything.

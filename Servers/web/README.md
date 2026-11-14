# PlusOS 2.0 Web & Wiki Server

Official distribution server and interactive technical Wiki for **PlusOS 2.0**, designed and created by **the1Domo**.

Configured for target domain: **`plusos.d0a.net`**

---

## Features

- **Zero Dependencies**: Pure Python 3 (standard library only). Runs on Linux, macOS, and Windows without `pip install`.
- **System 7 Retro Aesthetic**: Classic Macintosh UI design with modern responsive layout, authentic titlebars, pinstripes, Chicago font, and CRT green/amber toggle.
- **Interactive Technical Wiki**: Full markdown rendering for all 12 system documentation articles in its own dedicated `wiki/` directory.
- **In-Browser Wiki Editor**: Built-in management interface to add new articles or edit existing technical documentation directly from your server.
- **Disk Image Distribution**: Serves pre-configured `.hda` disk images and source code archives.
- **Production Ready**: Includes systemd service, Nginx reverse proxy template, and Caddyfile for automatic HTTPS.

---

## Directory Structure

```text
web/
├── server.py              # Main HTTP & Wiki API server
├── config.json            # Server settings (port, domain, admin key)
├── run.sh                 # Linux / macOS launcher script
├── run.bat                # Windows launcher script
├── plusos-web.service     # Systemd background service definition
├── nginx.conf.example     # Nginx reverse proxy configuration for plusos.d0a.net
├── caddy.conf.example     # Caddy configuration with automatic SSL
├── static/
│   ├── style.css          # System 7 retro-modern CSS stylesheet
│   └── app.js             # Client scripts (search, CRT toggle, editor)
├── templates/
│   ├── index.html         # Homepage & download portal
│   ├── wiki.html          # Interactive Wiki reader
│   └── wiki_edit.html     # Browser-based Wiki article editor
├── wiki/                  # The operating system Wiki in its own folder
│   ├── index.json         # Article catalog and metadata
│   ├── 01-getting-started.md
│   ├── 02-hardware-support.md
│   ├── 03-bootloader-and-startup.md
│   ├── 04-kernel-architecture.md
│   ├── 05-memory-management-and-gc.md
│   ├── 06-multitasking-and-ipc.md
│   ├── 07-desktop-and-window-server.md
│   ├── 08-classic-mac-loader.md
│   ├── 09-networking-plusnet.md
│   ├── 10-graphics-and-3d-engine.md
│   ├── 11-syscall-abi-reference.md
│   └── 12-sdk-and-app-development.md
└── downloads/             # Place downloadable PlusOS.hda and source files here
```

---

## Quick Start: Running the Server

### Option A: Local Testing
Run with Python 3:
```bash
python3 server.py --port 8080
```
Then open your browser to:
- Homepage: `http://localhost:8080`
- Technical Wiki: `http://localhost:8080/wiki`
- Add New Article: `http://localhost:8080/wiki/new`

### Option B: Deploying to your Server System (e.g. Ubuntu / Debian)

1. Copy the `web/` directory to `/var/www/plusos/web`:
   ```bash
   sudo mkdir -p /var/www/plusos
   sudo cp -r web /var/www/plusos/
   ```

2. Copy the disk image to the downloads directory:
   ```bash
   cp images/PlusOS.hda /var/www/plusos/web/downloads/
   ```

3. Install the systemd service:
   ```bash
   sudo cp /var/www/plusos/web/plusos-web.service /etc/systemd/system/
   sudo systemctl daemon-reload
   sudo systemctl enable --now plusos-web
   ```

4. Set up Nginx reverse proxy for `plusos.d0a.net`:
   ```bash
   sudo cp /var/www/plusos/web/nginx.conf.example /etc/nginx/sites-available/plusos.d0a.net
   sudo ln -s /etc/nginx/sites-available/plusos.d0a.net /etc/nginx/sites-enabled/
   sudo nginx -t && sudo systemctl reload nginx
   ```

5. Secure with Let's Encrypt SSL:
   ```bash
   sudo certbot --nginx -d plusos.d0a.net
   ```

---

## Managing Wiki Articles

- **View Wiki**: Navigate to `https://plusos.d0a.net/wiki`
- **Add Article**: Click **+ New Article** or visit `/wiki/new`
- **Edit Article**: Click **✏️ Edit Article** on any documentation page
- **Admin Key**: Protected by the password defined in `config.json` (default: `plusos2026`). Change this key in `config.json` before exposing the site publicly!

# PlusOS Standalone Package Repository Server

A dedicated, self-contained binary package repository and distribution server for PlusOS. Completely isolated from any web servers, with native support for both **SSL/HTTPS** (for modern browsers and Cloudflare-protected sites) and **plain HTTP** (for vintage 68000 Macintosh hardware and local self-hosters).

---

## Quick Start

### Windows
```cmd
cd repository
run_repo.bat
```

### Linux / WSL / macOS
```bash
cd repository
chmod +x run_repo.sh
./run_repo.sh
```

By default, the server binds to:
- **HTTP (Port `8088`)**: For vintage 68k Macintosh computers running PlusOS (`pkg` / `pkgmgr`) and local intranet users without SSL.
- **HTTPS (Port `8443`)**: For modern web browsers and secure connections. Automatically generates a 2048-bit RSA self-signed certificate in `certs/` if custom certificates are not provided.

---

## Server Options & Arguments

```bash
python server.py [options]

Options:
  --host HOST          Host interface to bind to (default: 0.0.0.0)
  --port PORT          Plain HTTP port for 68k clients (default: 8088)
  --ssl-port SSL_PORT  HTTPS / SSL port for browsers (default: 8443)
  --dual               Run both HTTP and HTTPS listeners simultaneously (default: True)
  --cert CERT          Path to SSL certificate file (default: certs/cert.pem)
  --key KEY            Path to SSL private key file (default: certs/key.pem)
  --upstream URL       Upstream HTTPS repo URL for proxying & caching (e.g. https://repo.plusos.d0a.net)
  --git URL            Git repository URL to automatically clone/pull packages from
```

---

## Cloudflare SSL & Reverse Proxy Setup

If you are hosting your repository behind **Cloudflare**:
1. Point your domain (e.g. `repo.mydomain.com`) to your server IP with Cloudflare proxy enabled (Orange Cloud).
2. Under Cloudflare SSL/TLS settings:
   - **Flexible SSL**: Cloudflare connects to your server via HTTP (`--port 8088`), and serves HTTPS to the world.
   - **Full / Strict SSL**: Cloudflare connects to your server via HTTPS (`--ssl-port 8443`) using your certificate or an Origin CA certificate.
3. The server automatically detects Cloudflare headers (`CF-Connecting-IP`, `X-Forwarded-Proto`, `X-Forwarded-For`).

---

## Self-Hosting & Custom Repositories

Users who set up their own repository do not need to rely on the official server:
1. Place any compiled `.pex` binaries into `repository/packages/`.
2. Run `server.py` on your machine or local LAN.
3. On your Macintosh running PlusOS, configure your repository host:
   ```sh
   pkg config repo 192.168.1.100:8088
   pkg update
   pkg list
   ```

### Upstream Proxy Mode
If you run a local repository server on your LAN, you can tell it to mirror an upstream Cloudflare HTTPS repository:
```bash
python server.py --port 8088 --upstream https://repo.plusos.d0a.net
```
When your vintage 68k Mac requests a package over plain HTTP, the local server fetches it securely over HTTPS from the upstream Cloudflare site, caches it locally in `packages/`, and serves it to the Mac.

---

## PlusOS Client Software

- **CLI Package Manager (`apps/network/pkg/pkg.c`)**:
  - `pkg update` - Fetch catalog from repository
  - `pkg list` - List available & installed packages
  - `pkg search <term>` - Search catalog
  - `pkg install <name>` - Download binary directly to `:bin:` or `:Games:`
  - `pkg remove <name>` - Delete package from disk
  - `pkg config repo <ip[:port]>` - Configure repository host
- **GUI Software Center (`apps/system/pkgmgr/pkgmgr.c`)**:
  - Graphical desktop interface with category filters (`All`, `Games`, `Dev`, `Net`, `Tools`).
  - Interactive one-click `[Install / Download]`, `[Launch]`, and `[Remove]` buttons.

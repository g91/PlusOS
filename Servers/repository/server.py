#!/usr/bin/env python3
"""
PlusOS Package Repository Server (APT / YUM style system)
Independent server with native Dual HTTP & HTTPS (SSL / Cloudflare) support.

Features:
- Dual Listener: Runs both plain HTTP (for vintage 68k Mac clients) and HTTPS (for SSL browsers & Cloudflare)
- Full Cloudflare SSL & Reverse Proxy integration (X-Forwarded-Proto, CF-Connecting-IP)
- Built-in self-signed certificate generation if custom SSL certs are not provided
- Upstream proxy/cache mode: can mirror an upstream Cloudflare HTTPS repo and serve to local vintage Macs over HTTP
- Standalone Git repository sync support (pulls and updates packages automatically)
- Serves pipe-delimited index.txt (for 68000 systems), index.json, binary .pex files, and web portal dashboard
"""

import os
import sys
import json
import time
import socket
import ssl
import subprocess
import argparse
import threading
from pathlib import Path
from http.server import HTTPServer, BaseHTTPRequestHandler
from urllib.parse import urlparse, parse_qs, unquote
from urllib.request import urlopen, Request

REPO_DIR = Path(__file__).resolve().parent
PROJECT_DIR = REPO_DIR.parent
PACKAGES_DIR = REPO_DIR / "packages"
CERTS_DIR = REPO_DIR / "certs"

# Catalog metadata database for PlusOS apps
PACKAGE_METADATA = {
    "ASTEROIDS": {"desc": "Vector Asteroids arcade game with physics", "cat": "games"},
    "BASIC": {"desc": "Retro BASIC interpreter with graphics & math", "cat": "development"},
    "BOOT": {"desc": "Startup disk selector & SCSI chainbooter", "cat": "system"},
    "BREAKOUT": {"desc": "Paddle and brick breakout arcade game", "cat": "games"},
    "CALC": {"desc": "Desktop floating-point calculator", "cat": "tools"},
    "CAT": {"desc": "Concatenate and print text files", "cat": "tools"},
    "CHIP8": {"desc": "CHIP-8 virtual machine & retro arcade emulator", "cat": "development"},
    "CLEAR": {"desc": "Clear terminal console screen", "cat": "tools"},
    "CLIP": {"desc": "Clipboard scrap viewer and text buffer", "cat": "tools"},
    "CUBE3D": {"desc": "Real-time 3D wireframe & solid spinning cube", "cat": "development"},
    "APPLE2E": {"desc": "Apple //e 65C02 computer & Hi-Res emulator", "cat": "development"},
    "C64": {"desc": "Commodore 64 MOS 6510 / VIC-II / SID emulator", "cat": "development"},
    "C128": {"desc": "Commodore 128 dual 40/80 column 128K emulator", "cat": "development"},
    "DATE": {"desc": "Display or set current date and time", "cat": "tools"},
    "DESKTOP": {"desc": "Graphical window manager & theme compositor", "cat": "system"},
    "DMESG": {"desc": "Dump kernel boot and driver log messages", "cat": "tools"},
    "DUNGEON": {"desc": "Roguelike dungeon crawler adventure RPG", "cat": "games"},
    "ECHO": {"desc": "Print arguments to console", "cat": "tools"},
    "EMU6502": {"desc": "MOS 6502 CPU and Apple I Woz Monitor", "cat": "development"},
    "EMU8080": {"desc": "Intel 8080 CPU & CP/M 2.2 subsystem", "cat": "development"},
    "FILES": {"desc": "Interactive dual-pane HFS file manager", "cat": "tools"},
    "FLAPPY": {"desc": "Flappy Mac side-scrolling arcade challenge", "cat": "games"},
    "GC": {"desc": "Kernel garbage collector stats and cycle", "cat": "tools"},
    "HELLO": {"desc": "Minimal SDK hello world demo", "cat": "development"},
    "HELP": {"desc": "List all commands and user guides", "cat": "tools"},
    "HEXEDIT": {"desc": "Hexadecimal binary file editor & viewer", "cat": "tools"},
    "INSTALLER": {"desc": "PlusOS Hard Drive Installation Utility", "cat": "system"},
    "INVADERS": {"desc": "Space Invaders arcade defense game", "cat": "games"},
    "IRC": {"desc": "Internet Relay Chat client over BlueSCSI", "cat": "network"},
    "KILL": {"desc": "Terminate process by PID", "cat": "tools"},
    "LS": {"desc": "List files and directories", "cat": "tools"},
    "LUNAR": {"desc": "Lunar Lander physics simulation", "cat": "games"},
    "MACRUN": {"desc": "Classic Mac OS APPL executable loader", "cat": "development"},
    "MEM": {"desc": "Display free and used RAM statistics", "cat": "tools"},
    "MINES": {"desc": "Minesweeper puzzle game", "cat": "games"},
    "NET": {"desc": "TCP/IP configuration and DaynaPORT stats", "cat": "network"},
    "NOTES": {"desc": "Notepad text editor with persistent storage", "cat": "tools"},
    "OFF": {"desc": "Power off or safe shutdown system", "cat": "system"},
    "PACMAN": {"desc": "Classic maze ghost-chase arcade game", "cat": "games"},
    "PING": {"desc": "ICMP network ping utility", "cat": "network"},
    "PKG": {"desc": "APT/YUM style package manager CLI", "cat": "network"},
    "PKGMGR": {"desc": "Graphical software center package manager", "cat": "system"},
    "PS": {"desc": "List running processes and thread states", "cat": "tools"},
    "PUZZLE": {"desc": "15-tile sliding number puzzle DA", "cat": "games"},
    "QDEMO": {"desc": "QuickDraw shapes and pattern showcase", "cat": "development"},
    "RACER": {"desc": "Top-down retro road racing game", "cat": "games"},
    "RAM": {"desc": "Display RAM memory layout and heap bars", "cat": "tools"},
    "REBOOT": {"desc": "Warm restart computer", "cat": "system"},
    "REG": {"desc": "System registry viewer and modifier", "cat": "tools"},
    "RET": {"desc": "Return to Mac OS from PlusOS", "cat": "system"},
    "SHELL": {"desc": "Interactive command line interpreter", "cat": "system"},
    "SLEEP": {"desc": "Delay execution for given seconds", "cat": "tools"},
    "SNAKE": {"desc": "Classic nibbles snake arcade game", "cat": "games"},
    "SOLARSYS": {"desc": "Solar system planetary orbit simulation", "cat": "development"},
    "SOUND": {"desc": "4-voice PWM audio synthesizer test", "cat": "tools"},
    "SPIN": {"desc": "Real-time 3D solid mesh rotating model", "cat": "development"},
    "STARFLIGHT": {"desc": "Deep space exploration and galaxy combat RPG", "cat": "games"},
    "STARTUP": {"desc": "Configure default boot application", "cat": "system"},
    "SYSINFO": {"desc": "CPU, ROM, screen, and hardware audit", "cat": "tools"},
    "TASKMAN": {"desc": "Live CPU and memory usage task manager", "cat": "tools"},
    "TELNET": {"desc": "Telnet network terminal client", "cat": "network"},
    "TETRIS": {"desc": "Classic falling block puzzle game", "cat": "games"},
    "THEME": {"desc": "Multi-OS visual theme selector", "cat": "tools"},
    "TOP": {"desc": "Live system monitor and process viewer", "cat": "tools"},
    "UPTIME": {"desc": "Show system running time and ticks", "cat": "tools"},
    "VER": {"desc": "Display PlusOS kernel and SDK version", "cat": "tools"},
}

class PkgManager:
    def __init__(self, upstream_url=None, git_url=None):
        self.upstream_url = upstream_url
        self.git_url = git_url
        self.packages = []
        PACKAGES_DIR.mkdir(parents=True, exist_ok=True)
        self.refresh()

    def sync_git(self):
        if not self.git_url:
            return False, "No Git repository configured."
        try:
            if (PACKAGES_DIR / ".git").exists():
                r = subprocess.run(["git", "pull"], cwd=PACKAGES_DIR, capture_output=True, text=True)
            else:
                r = subprocess.run(["git", "clone", self.git_url, str(PACKAGES_DIR)], capture_output=True, text=True)
            self.refresh()
            return r.returncode == 0, r.stdout + "\n" + r.stderr
        except Exception as e:
            return False, str(e)

    def refresh(self):
        self.packages = []
        seen = set()

        # 1. Check local packages/ folder
        if PACKAGES_DIR.exists():
            for f in sorted(os.listdir(PACKAGES_DIR)):
                if f.endswith(".pex"):
                    name = f[:-4].upper()
                    seen.add(name.lower())
                    sz = os.path.getsize(PACKAGES_DIR / f)
                    meta = PACKAGE_METADATA.get(name, {"desc": f"PlusOS application {name}", "cat": "tools"})
                    self.packages.append({
                        "name": name.lower(),
                        "filename": f,
                        "version": "1.0",
                        "category": meta["cat"],
                        "size": sz,
                        "description": meta["desc"],
                        "source": "local"
                    })

        # 2. Check project build/apps/ folder (development builds)
        build_apps = PROJECT_DIR / "build" / "apps"
        if build_apps.exists():
            for f in sorted(os.listdir(build_apps)):
                if f.endswith(".pex"):
                    name = f[:-4].upper()
                    if name.lower() not in seen:
                        seen.add(name.lower())
                        sz = os.path.getsize(build_apps / f)
                        meta = PACKAGE_METADATA.get(name, {"desc": f"PlusOS application {name}", "cat": "tools"})
                        self.packages.append({
                            "name": name.lower(),
                            "filename": f,
                            "version": "1.0",
                            "category": meta["cat"],
                            "size": sz,
                            "description": meta["desc"],
                            "source": "build"
                        })

        # Write index.json and index.txt into packages directory
        with open(PACKAGES_DIR / "index.json", "w", encoding="utf-8") as jf:
            json.dump(self.packages, jf, indent=2)

        with open(PACKAGES_DIR / "index.txt", "w", encoding="utf-8", newline="\r\n") as tf:
            for p in self.packages:
                tf.write(f"{p['name']}|{p['version']}|{p['category']}|{p['size']}|{p['description']}\r\n")

        print(f"[Repo] Indexed {len(self.packages)} packages successfully.")

    def get_package_bytes(self, filename):
        # 1. Local packages folder
        p = PACKAGES_DIR / filename
        if p.exists():
            with open(p, "rb") as f:
                return f.read()

        # 2. Project build/apps
        p = PROJECT_DIR / "build" / "apps" / filename
        if p.exists():
            with open(p, "rb") as f:
                return f.read()

        # 3. If upstream configured, fetch from upstream (Cloudflare HTTPS proxy/cache)
        if self.upstream_url:
            url = f"{self.upstream_url.rstrip('/')}/packages/{filename}"
            print(f"[Repo] Proxying from upstream: {url}")
            try:
                req = Request(url, headers={"User-Agent": "PlusOS-Repo-Bridge/1.0"})
                with urlopen(req, timeout=15) as resp:
                    data = resp.read()
                    # Cache locally
                    with open(PACKAGES_DIR / filename, "wb") as f:
                        f.write(data)
                    return data
            except Exception as e:
                print(f"[Repo] Upstream fetch failed: {e}")

        return None

pkg_mgr = None

class RepoRequestHandler(BaseHTTPRequestHandler):
    def log_message(self, format, *args):
        client_ip = self.headers.get("CF-Connecting-IP") or self.headers.get("X-Forwarded-For") or self.client_address[0]
        proto = "HTTPS" if getattr(self.server, "is_ssl", False) or self.headers.get("X-Forwarded-Proto") == "https" else "HTTP"
        sys.stderr.write(f"[{proto}] [{client_ip}] {format % args}\n")

    def do_GET(self):
        parsed = urlparse(self.path)
        path = unquote(parsed.path)

        # 1. Line-based index for 68000 systems (PlusOS pkg & pkgmgr)
        if path == "/packages/index.txt":
            body = (PACKAGES_DIR / "index.txt").read_bytes() if (PACKAGES_DIR / "index.txt").exists() else b""
            self.send_response(200)
            self.send_header("Content-Type", "text/plain; charset=utf-8")
            self.send_header("Content-Length", str(len(body)))
            self.send_header("Access-Control-Allow-Origin", "*")
            self.end_headers()
            self.wfile.write(body)
            return

        # 2. JSON API
        if path in ("/api/packages", "/packages/index.json"):
            body = json.dumps(pkg_mgr.packages, indent=2).encode("utf-8")
            self.send_response(200)
            self.send_header("Content-Type", "application/json; charset=utf-8")
            self.send_header("Content-Length", str(len(body)))
            self.send_header("Access-Control-Allow-Origin", "*")
            self.end_headers()
            self.wfile.write(body)
            return

        # 3. Binary .pex download
        if path.startswith("/packages/") and path.endswith(".pex"):
            filename = path[len("/packages/"):]
            data = pkg_mgr.get_package_bytes(filename)
            if not data:
                # Try lowercase / uppercase variant
                data = pkg_mgr.get_package_bytes(filename.lower())
            if not data:
                data = pkg_mgr.get_package_bytes(filename.upper())

            if data:
                self.send_response(200)
                self.send_header("Content-Type", "application/octet-stream")
                self.send_header("Content-Disposition", f'attachment; filename="{filename}"')
                self.send_header("Content-Length", str(len(data)))
                self.send_header("Access-Control-Allow-Origin", "*")
                self.end_headers()
                self.wfile.write(data)
                return
            else:
                self.send_error(404, f"Package binary {filename} not found.")
                return

        # 4. Web portal dashboard for browsers
        if path in ("/", "/packages", "/packages/"):
            html = self.render_portal_html()
            body = html.encode("utf-8")
            self.send_response(200)
            self.send_header("Content-Type", "text/html; charset=utf-8")
            self.send_header("Content-Length", str(len(body)))
            self.end_headers()
            self.wfile.write(body)
            return

        self.send_error(404, "Not Found")

    def do_POST(self):
        parsed = urlparse(self.path)
        path = unquote(parsed.path)

        if path == "/api/packages/sync":
            content_length = int(self.headers.get("Content-Length", 0))
            data = {}
            if content_length > 0:
                try:
                    data = json.loads(self.rfile.read(content_length).decode("utf-8"))
                except Exception:
                    pass

            if data.get("git"):
                ok, msg = pkg_mgr.sync_git()
            else:
                pkg_mgr.refresh()
                ok, msg = True, f"Catalog refreshed with {len(pkg_mgr.packages)} packages."

            res = json.dumps({"ok": ok, "message": msg, "count": len(pkg_mgr.packages)}).encode("utf-8")
            self.send_response(200 if ok else 500)
            self.send_header("Content-Type", "application/json; charset=utf-8")
            self.send_header("Content-Length", str(len(res)))
            self.send_header("Access-Control-Allow-Origin", "*")
            self.end_headers()
            self.wfile.write(res)
            return

        self.send_error(404, "Endpoint Not Found")

    def render_portal_html(self):
        rows = []
        for p in pkg_mgr.packages:
            sz_kb = f"{p['size'] / 1024.0:.1f} KB" if p['size'] > 0 else "0 KB"
            badge = f'<span class="badge cat-{p["category"]}">{p["category"].upper()}</span>'
            cmd = f'pkg install {p["name"]}'
            dl = f'<a href="/packages/{p["filename"]}" class="btn-dl">Download .pex</a>'
            rows.append(f"""
            <tr>
              <td><strong class="pkg-name">{p['name']}</strong></td>
              <td>{p['version']}</td>
              <td>{badge}</td>
              <td>{sz_kb}</td>
              <td>{p['description']}</td>
              <td><code class="cmd">{cmd}</code></td>
              <td style="text-align:right;">{dl}</td>
            </tr>
            """)

        rows_html = "".join(rows)
        proto = "HTTPS (SSL Active)" if getattr(self.server, "is_ssl", False) or self.headers.get("X-Forwarded-Proto") == "https" else "HTTP"
        upstream_info = f"<div><strong>Upstream Proxy:</strong> {pkg_mgr.upstream_url}</div>" if pkg_mgr.upstream_url else ""

        return f"""<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <title>PlusOS Package Repository</title>
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <style>
    body {{ background: #0c0e12; color: #d1d5db; font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, monospace; margin: 0; padding: 20px; }}
    .container {{ max-width: 1100px; margin: 0 auto; }}
    .header {{ background: #161b22; border: 1px solid #30363d; border-radius: 6px; padding: 20px; margin-bottom: 20px; }}
    h1 {{ color: #58a6ff; margin: 0 0 10px 0; font-size: 24px; }}
    .status-bar {{ display: flex; gap: 20px; font-size: 13px; color: #8b949e; margin-top: 10px; }}
    .status-bar span {{ color: #3fb950; font-weight: bold; }}
    .cmd-box {{ background: #0d1117; border: 1px solid #30363d; border-radius: 6px; padding: 15px; margin-bottom: 20px; font-family: monospace; font-size: 13px; line-height: 1.6; color: #c9d1d9; }}
    .cmd-box strong {{ color: #58a6ff; }}
    table {{ width: 100%; border-collapse: collapse; background: #161b22; border: 1px solid #30363d; border-radius: 6px; overflow: hidden; }}
    th {{ background: #0d1117; color: #8b949e; text-align: left; padding: 10px 12px; font-size: 12px; border-bottom: 1px solid #30363d; }}
    td {{ padding: 10px 12px; border-bottom: 1px solid #21262d; font-size: 13px; }}
    tr:hover {{ background: #1f242c; }}
    .pkg-name {{ color: #58a6ff; font-family: monospace; }}
    .badge {{ display: inline-block; padding: 2px 6px; border-radius: 3px; font-size: 10px; font-weight: bold; text-transform: uppercase; background: #21262d; border: 1px solid #30363d; color: #c9d1d9; }}
    .badge.cat-games {{ color: #bc8cff; border-color: #8957e5; }}
    .badge.cat-development {{ color: #58a6ff; border-color: #1f6feb; }}
    .badge.cat-network {{ color: #3fb950; border-color: #238636; }}
    .badge.cat-system {{ color: #f0883e; border-color: #bd561d; }}
    .cmd {{ background: #0d1117; padding: 2px 6px; border-radius: 3px; border: 1px solid #30363d; color: #3fb950; }}
    .btn-dl {{ background: #238636; color: #fff; text-decoration: none; padding: 4px 10px; border-radius: 4px; font-size: 12px; font-weight: bold; display: inline-block; }}
    .btn-dl:hover {{ background: #2ea043; }}
    .btn-sync {{ background: #1f6feb; color: #fff; border: none; padding: 6px 12px; border-radius: 4px; font-weight: bold; cursor: pointer; }}
    .btn-sync:hover {{ background: #388bfd; }}
  </style>
  <script>
    function syncCatalog() {{
      fetch('/api/packages/sync', {{ method: 'POST' }})
        .then(function(r) {{ return r.json(); }})
        .then(function(d) {{ alert(d.message); location.reload(); }})
        .catch(function(e) {{ alert('Sync failed: ' + e); }});
    }}
  </script>
</head>
<body>
  <div class="container">
    <div class="header">
      <div style="display:flex; justify-content:space-between; align-items:center;">
        <div>
          <h1>PlusOS Binary Package Repository</h1>
          <div style="color:#8b949e; font-size:14px;">Standalone package management and distribution server for PlusOS.</div>
        </div>
        <div>
          <button onclick="syncCatalog()" class="btn-sync">↻ Rebuild Index</button>
        </div>
      </div>
      <div class="status-bar">
        <div>Total Packages: <span>{len(pkg_mgr.packages)}</span></div>
        <div>Connection Security: <span>{proto}</span></div>
        <div>68k Protocol Index: <a href="/packages/index.txt" style="color:#58a6ff;">/packages/index.txt</a></div>
        <div>JSON API: <a href="/api/packages" style="color:#58a6ff;">/api/packages</a></div>
        {upstream_info}
      </div>
    </div>

    <div class="cmd-box">
      <strong>PlusOS Client Quick Commands:</strong><br>
      &bull; <code>pkg update</code> &mdash; Refresh package index cache from this repository<br>
      &bull; <code>pkg list</code> &mdash; Show all available and installed packages<br>
      &bull; <code>pkg install &lt;package&gt;</code> &mdash; Download executable binary directly onto disk<br>
      &bull; <code>pkgmgr</code> &mdash; Launch graphical Software Center window on PlusOS Desktop<br>
      &bull; <code>pkg config repo &lt;host[:port]&gt;</code> &mdash; Point client to a custom or self-hosted repository
    </div>

    <table>
      <thead>
        <tr>
          <th>Package</th>
          <th>Version</th>
          <th>Category</th>
          <th>Size</th>
          <th>Description</th>
          <th>Install Command</th>
          <th style="text-align:right;">Action</th>
        </tr>
      </thead>
      <tbody>
        {rows_html}
      </tbody>
    </table>
  </div>
</body>
</html>
"""

def generate_self_signed_cert(cert_path, key_path):
    CERTS_DIR.mkdir(parents=True, exist_ok=True)
    if os.path.exists(cert_path) and os.path.exists(key_path):
        return True

    print(f"[SSL] Generating self-signed certificate in {CERTS_DIR}...")
    try:
        cmd = [
            "openssl", "req", "-x509", "-newkey", "rsa:2048",
            "-keyout", str(key_path), "-out", str(cert_path),
            "-days", "365", "-nodes",
            "-subj", "/CN=plusos-pkg-repo"
        ]
        r = subprocess.run(cmd, capture_output=True)
        if r.returncode == 0:
            print("[SSL] Self-signed certificate generated successfully.")
            return True
        else:
            print(f"[SSL] openssl command failed: {r.stderr.decode('utf-8', errors='ignore')}")
            return False
    except Exception as e:
        print(f"[SSL] Could not invoke openssl: {e}")
        return False

def run_http_server(host, port):
    httpd = HTTPServer((host, port), RepoRequestHandler)
    httpd.is_ssl = False
    print(f"[HTTP]  Listening on http://{host}:{port}/ (Vintage 68k clients & local users)")
    httpd.serve_forever()

def run_https_server(host, port, cert_file, key_file):
    httpd = HTTPServer((host, port), RepoRequestHandler)
    httpd.is_ssl = True

    ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
    ctx.load_cert_chain(certfile=cert_file, keyfile=key_file)
    httpd.socket = ctx.wrap_socket(httpd.socket, server_side=True)

    print(f"[HTTPS] Listening on https://{host}:{port}/ (SSL Browsers & Cloudflare)")
    httpd.serve_forever()

def main():
    global pkg_mgr

    parser = argparse.ArgumentParser(description="PlusOS Standalone Package Repository Server")
    parser.add_argument("--host", default="0.0.0.0", help="Host interface to bind to (default: 0.0.0.0)")
    parser.add_argument("--port", type=int, default=8088, help="Plain HTTP port for 68k clients (default: 8088)")
    parser.add_argument("--ssl-port", type=int, default=8443, help="HTTPS / SSL port for browsers (default: 8443)")
    parser.add_argument("--ssl", action="store_true", help="Enable SSL / HTTPS server")
    parser.add_argument("--dual", action="store_true", default=True, help="Run both HTTP and HTTPS listeners simultaneously (default: True)")
    parser.add_argument("--cert", default=str(CERTS_DIR / "cert.pem"), help="SSL certificate file path")
    parser.add_argument("--key", default=str(CERTS_DIR / "key.pem"), help="SSL private key file path")
    parser.add_argument("--upstream", default=None, help="Upstream HTTPS repository URL for proxying & caching")
    parser.add_argument("--git", default=None, help="Git repository URL to sync packages from")
    args = parser.parse_args()

    pkg_mgr = PkgManager(upstream_url=args.upstream, git_url=args.git)

    print("=" * 70)
    print("  PlusOS Independent Package Repository Server")
    print("=" * 70)
    print(f"  Packages Directory: {PACKAGES_DIR}")
    print(f"  Total Packages:     {len(pkg_mgr.packages)}")

    # Start HTTP listener in background thread
    http_thread = threading.Thread(target=run_http_server, args=(args.host, args.port), daemon=True)
    http_thread.start()

    # Check SSL
    has_ssl = False
    if args.ssl or args.dual:
        cert_p = Path(args.cert)
        key_p = Path(args.key)
        if not (cert_p.exists() and key_p.exists()):
            generate_self_signed_cert(cert_p, key_p)

        if cert_p.exists() and key_p.exists():
            https_thread = threading.Thread(target=run_https_server, args=(args.host, args.ssl_port, str(cert_p), str(key_p)), daemon=True)
            https_thread.start()
            has_ssl = True
        else:
            print("[WARN] SSL certificate not found and openssl not available. Running HTTP only.")

    print("=" * 70)
    print("Repository is active and ready.")
    print(f"  Vintage 68k / HTTP:  http://localhost:{args.port}/packages/index.txt")
    if has_ssl:
        print(f"  SSL Browser / HTTPS: https://localhost:{args.ssl_port}/")
    print("Press Ctrl+C to stop.")
    print("=" * 70)

    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        print("\nShutting down repository server...")

if __name__ == "__main__":
    main()

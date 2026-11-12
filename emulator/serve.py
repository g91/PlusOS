#!/usr/bin/env python3
"""serve.py - static file server for the PlusOS web emulator (Python standard library only).

    python emulator/serve.py [--port 8068] [--bind 127.0.0.1] [--mount /name=DIR ...]

Serves the REPOSITORY ROOT, so the page at /emulator/ can fetch the ROMs
(tools/emulator/vMac.ROM, ROM/SE/MacSE.ROM) and disk images (images/*.hda) by
relative URL. Correct MIME types for ES modules, no caching, and byte-range
requests are not needed (images are fetched whole).

--mount maps an extra directory into the URL space, e.g.
    --mount /scratch=C:/temp/images   ->   http://localhost:8068/scratch/test.hda
"""
import argparse
import http.server
import os
import posixpath
import sys
import urllib.parse

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
MOUNTS = {}

MIME = {
    ".html": "text/html; charset=utf-8", ".js": "text/javascript; charset=utf-8", ".mjs": "text/javascript; charset=utf-8",
    ".css": "text/css; charset=utf-8", ".json": "application/json", ".png": "image/png", ".svg": "image/svg+xml",
    ".md": "text/plain; charset=utf-8", ".txt": "text/plain; charset=utf-8", ".s": "text/plain; charset=utf-8",
    ".rom": "application/octet-stream", ".hda": "application/octet-stream", ".dsk": "application/octet-stream",
    ".img": "application/octet-stream", ".bin": "application/octet-stream", ".pex": "application/octet-stream",
}


class Handler(http.server.SimpleHTTPRequestHandler):
    def translate_path(self, path):
        path = urllib.parse.unquote(path.split("?", 1)[0].split("#", 1)[0])
        path = posixpath.normpath(path)
        base, rel = ROOT, path
        for prefix, target in MOUNTS.items():
            if path == prefix or path.startswith(prefix + "/"):
                base, rel = target, path[len(prefix):]
                break
        parts = [p for p in rel.split("/") if p and p not in (".", "..")]
        return os.path.join(base, *parts)

    def guess_type(self, path):
        ext = os.path.splitext(path)[1].lower()
        return MIME.get(ext) or super().guess_type(path)

    def end_headers(self):
        self.send_header("Cache-Control", "no-store, max-age=0")
        self.send_header("Cross-Origin-Opener-Policy", "same-origin")
        self.send_header("Cross-Origin-Embedder-Policy", "require-corp")
        super().end_headers()

    def do_GET(self):
        if self.path in ("/", ""):
            self.send_response(302)
            self.send_header("Location", "/emulator/")
            self.end_headers()
            return
        super().do_GET()

    def log_message(self, fmt, *args):
        if not QUIET:
            sys.stderr.write("%s - %s\n" % (self.address_string(), fmt % args))


QUIET = False


def main():
    global QUIET
    ap = argparse.ArgumentParser(description="Serve the PlusOS repository for the web emulator.")
    ap.add_argument("--port", type=int, default=8068)
    ap.add_argument("--bind", default="127.0.0.1", help="address to listen on (0.0.0.0 to share on the LAN)")
    ap.add_argument("--mount", action="append", default=[], metavar="/name=DIR")
    ap.add_argument("--quiet", action="store_true")
    a = ap.parse_args()
    QUIET = a.quiet
    for m in a.mount:
        name, _, target = m.partition("=")
        MOUNTS["/" + name.strip("/")] = os.path.abspath(target)
    srv = http.server.ThreadingHTTPServer((a.bind, a.port), Handler)
    print("PlusOS emulator: http://%s:%d/emulator/   (serving %s)" % ("localhost" if a.bind in ("127.0.0.1", "0.0.0.0") else a.bind, a.port, ROOT))
    try:
        srv.serve_forever()
    except KeyboardInterrupt:
        pass


if __name__ == "__main__":
    main()

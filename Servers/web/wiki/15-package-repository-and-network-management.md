# Package Repository Server & Software Center

PlusOS features a full APT/YUM-style software package management ecosystem designed specifically for vintage 68000 Macintosh hardware connected to the internet.

---

## Ecosystem Architecture

The package system consists of three distinct components:
1. **Dedicated Standalone Server (`repository/server.py`)**: A multi-protocol server running independently of the web frontend, hosting software packages, catalog manifests, and modern SSL/HTTPS proxying.
2. **`pkg` Command Line Interface (`apps/network/pkg/pkg.c`)**: Terminal-based package installer for quick installations from the PlusOS recovery shell.
3. **`pkgmgr` Graphical Software Center (`apps/system/pkgmgr/pkgmgr.c`)**: Full Desktop GUI application allowing users to browse, search, and install software packages with a single click.

```text
+----------------------------------------------------------------------------------+
|                            PlusOS Package Management                             |
+----------------------------------------------------------------------------------+
|                                                                                  |
|   Vintage Macintosh Hardware (Plus / SE)        Modern Browsers & Cloudflare     |
|   +------------------------------------+        +------------------------------+ |
|   | PlusOS Client Apps:                |        | HTTPS Web Browsers           | |
|   | - `pkg` CLI (Terminal)             |        | Cloudflare Proxy             | |
|   | - `pkgmgr` GUI (Software Center)   |        | Git Remote Sync              | |
|   +-----------------+------------------+        +--------------+---------------+ |
|                     |                                          |                 |
|                     | Plain HTTP (Port 8088)                   | SSL/HTTPS (8443)|
|                     v                                          v                 |
|   +----------------------------------------------------------------------------+ |
|   |                 Dedicated Repository Server (`repository/server.py`)       | |
|   |                                                                            | |
|   |   - Dual Listeners: HTTP (8088) + HTTPS / SSL (8443)                       | |
|   |   - Auto-Generated 2048-bit RSA Self-Signed Certificate                    | |
|   |   - Cloudflare Proxy Headers (CF-Connecting-IP, X-Forwarded-Proto)         | |
|   |   - Upstream Proxy Cache Mirroring (--upstream <url>)                      | |
|   |   - Live Web Portal with Search, Stats, & Download Links                   | |
|   |   - Package Catalog Indexer (`index.json` & `index.txt`)                   | |
|   +----------------------------------------------------------------------------+ |
|                                                                                  |
+----------------------------------------------------------------------------------+
```

---

## Dual Protocol Support: HTTP & HTTPS

Why two ports?
- **Plain HTTP (Default: Port `8088`)**:
  Vintage 68000 Macintosh hardware running at 8 MHz does not possess the processing power to negotiate modern TLS 1.3 cryptographic handshakes in real time. Serving packages over plain HTTP allows `pkg` and `pkgmgr` to download binaries in milliseconds over BlueSCSI DaynaPORT without SSL handshake overhead.
- **SSL / HTTPS (Default: Port `8443`)**:
  Modern web browsers, automated CI/CD runners, and Cloudflare-protected domains require end-to-end encrypted HTTPS. The server automatically generates a 2048-bit RSA certificate in `repository/certs/` (or accepts `--cert` and `--key`) to serve secure traffic simultaneously.

---

## Starting the Repository Server

The repository server is located in its own dedicated directory: `C:\Users\Evil\PlusOS\repository/`.

### Windows
```cmd
cd repository
run_repo.bat
```

### Linux / WSL / macOS
```bash
cd repository
./run_repo.sh
```

### Command-Line Arguments
```text
python3 repository/server.py [options]

Options:
  --port, -p PORT         HTTP listener port (default: 8088)
  --ssl-port SSL_PORT     HTTPS listener port (default: 8443)
  --host HOST             Bind address (default: 0.0.0.0)
  --packages-dir DIR      Directory holding .pex packages (default: repository/packages)
  --no-ssl                Disable HTTPS/SSL listener
  --cert CERT_FILE        Path to SSL certificate (.crt/.pem)
  --key KEY_FILE          Path to SSL private key (.key)
  --upstream URL          Upstream repository URL to mirror and cache locally
```

---

## Client Usage inside PlusOS

### 1. `pkg` Command Line Interface
```bash
# Update package catalog index
pkg update

# List all available software in the repository
pkg list

# Search for a specific application
pkg search tetris

# Download and install an application into :bin:
pkg install asteroids

# Show detailed metadata and package size
pkg info cube3d
```

### 2. `pkgmgr` Graphical Software Center
From the PlusOS Desktop, launch **Package Manager** (`:bin:PKGMGR` or via the **Programs** launcher):
- Browse packages organized by category (System, Tools, Games, Development, Network).
- Search with real-time filtering as you type.
- Click **Install** to download and write the application directly into your `:bin:` folder.
- Inspect binary sizes, version numbers, and descriptions before downloading.

---

## Navigation

- **Previous Chapter**: [← 14. Minimal Bootable Floppy & Hard Drive Installer](/wiki/14-bootable-floppy-and-installer)
- **Tooling Suite**: [13. PlusOS Disk Studio & Compiler Pipeline →](/wiki/13-plusos-disk-studio-and-compiler)
- **Table of Contents**: [Return to Chapter 1: Getting Started](/wiki/01-getting-started)

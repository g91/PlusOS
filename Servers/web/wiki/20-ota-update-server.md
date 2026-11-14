# OTA Update Server: Over-the-Air Updates

PlusOS can update itself over the internet. The **OTA (Over-The-Air) Update Server** lets the maintainer publish a new release every time they compile, and lets users pull the new kernel, applications, and repositories and apply them to an existing disk or BlueSCSI SD card **without reflashing** — all your files are kept. It is a standalone service, reachable through Cloudflare at **https://ota.d0a.net**, and it ships with an AI assistant that writes a friendly description of every update.

---

## Why OTA?

Before OTA, every change meant rebuilding a whole disk image and writing it back to the SD card from scratch — wiping your documents, settings, and Desktop. The OTA system solves that:

- **The maintainer** publishes once per compile; the server stores only what actually changed (content-addressed, so an unchanged file is never re-uploaded).
- **Users** see exactly which updates exist, read what each one does, choose *Update All* or pick individual items, and apply them in place. Home folders, `etc`, documents, and Desktop items survive, and the disk's Apple SCSI boot driver is reused.

```text
+--------------------------------------------------------------------------------+
|                           PlusOS OTA Update System                             |
+--------------------------------------------------------------------------------+
|                                                                                |
|   MAINTAINER (one machine)                 USERS (anywhere)                     |
|   +-----------------------------+          +--------------------------------+   |
|   | Compile (make / Disk Studio)|          | Web client GUI  (browser)      |   |
|   | -> publish_cli.py (SIGNED)  |          | PlusOS Updater  (host GUI)     |   |
|   |    + AI release notes       |          | plusupdate      (native 68k)   |   |
|   +--------------+--------------+          +----------------+---------------+   |
|                  |  signed publish                          |  catalog / chat   |
|                  v        (HTTPS/WSS via Cloudflare)         v                   |
|   +--------------------------------------------------------------------------+   |
|   |          OTA Server  (FastAPI + WebSocket + SQLite)  :8799                |   |
|   |   - releases, artifacts, AI docs, chat, audit log                        |   |
|   |   - signed publishing (only the maintainer's key can commit)             |   |
|   |   - admin control panel  (/admin)                                        |   |
|   |   - native line-protocol bridge  :8990  (for the 68k app, LAN only)      |   |
|   +--------------------------------------------------------------------------+   |
|                                                                                |
+--------------------------------------------------------------------------------+
```

---

## The three ways to receive updates

| Client | Runs on | Reaches server via | Can apply updates? |
|:---|:---|:---|:---|
| **Web client GUI** (`/`) | Any modern browser | `https://ota.d0a.net` | Downloads artifacts; apply with the Updater |
| **PlusOS Updater** | Your PC (host-side GUI) | `https://ota.d0a.net` | **Yes** — runs `hdbuild update` for you |
| **`plusupdate`** | Inside PlusOS (native 68k app) | LAN TCP bridge `:8990` | Browse + AI chat (read-only for now) |

> **Why three?** PlusOS has no HTTP/TLS stack, so the in-OS app can't speak to Cloudflare directly. It talks a tiny line protocol over raw TCP to the server's bridge on your local network (exactly like the IKANET chat client). The heavy lifting of writing a disk image happens on a real PC through the **PlusOS Updater**.

> **No network?** If the machine is in front of you, you don't need the server at all. The **Update** button in [PlusOS Disk Studio](/wiki/13-plusos-disk-studio-and-compiler) recompiles the OS from source and refreshes your disk or BlueSCSI card in place — same preserve-your-files, reuse-the-driver guarantees, entirely offline. OTA is the remote path; Disk Studio's Update is the local one.

---

## For users: applying an update with PlusOS Updater

The **PlusOS Updater** is a small host-side GUI (`Servers/otaserver/updater/plusos_updater.py`). It needs nothing but a normal Python install.

```cmd
python Servers\otaserver\updater\plusos_updater.py
```

1. **Server** — leave it on `https://ota.d0a.net` (or your own server URL).
2. **Channel** — `stable`, `beta`, or `nightly`.
3. **Machine** — your Mac: `se`, `plus`, `classic`, `lc3`, or `universal`.
4. **Disk / SD card** — browse to your PlusOS `.hda` image or the BlueSCSI card's `.hda`.
5. Click **Check for Updates**. Releases newer than what you've applied are marked **NEW**; select one to read its notes.
6. **Select All New** (or tick individual items), then **Update Selected**.

The Updater downloads the chosen artifacts and runs `hdbuild update`, which refreshes the kernel, boot blocks, and applications while preserving your user data and reusing the disk's SCSI driver. See also [Minimal Bootable Floppy & Hard Drive Installer](/wiki/14-bootable-floppy-and-installer) and the [Disk Studio & Compiler Pipeline](/wiki/13-plusos-disk-studio-and-compiler).

---

## For users: the web client

Open **https://ota.d0a.net/** in any browser. The System-7-styled page lists every release, shows the AI-written description and the items inside, and flags what's new since you last looked. Pick a channel and machine, tick what you want, and **Download Selected** to grab the raw artifacts. To actually install them on a disk, use the PlusOS Updater above.

---

## For users: `plusupdate` inside PlusOS

A native 1-bit Macintosh application (`:bin:PLUSUPDATE`) that shows the update catalog and lets you chat with the Update Assistant from inside the OS.

```text
plusupdate [SERVER] [PORT] [CHANNEL]      defaults: ota.local 8990 stable
```

Because PlusOS networking only runs on real hardware with a **BlueSCSI DaynaPORT** (see [PlusNET](/wiki/09-networking-plusnet)), point it at the LAN address of the machine running the OTA server, e.g. `plusupdate 192.168.1.50 8990`. It lists releases, shows each one's AI message and contents, and has a **Chat** view to ask the assistant questions. Applying an update from inside PlusOS is not yet supported (the running boot volume can't rewrite itself); `plusupdate` marks a release as *wanted* so the PlusOS Updater applies it on the next sync.

---

## For the maintainer: publishing a release

Publishing is gated two independent ways, so only you can push software:

1. **A signing key** (`Servers/otaserver/.publish_key`) that lives only on your machine. Every publish is an HMAC challenge bound to a timestamp — the key never travels over the wire, and a captured request can't be replayed.
2. **An admin password** for the control panel (a separate gate; the password can't publish and the key can't open the panel).

### From the command line / Makefile

```cmd
make.cmd ota OTA_SERVER=https://ota.d0a.net OTA_CHANNEL=stable OTA_TITLE="Desktop speedups"
```

This builds for the current `MACHINE`, then runs the publisher
(`Servers/otaserver/publish_cli.py`), which:

- discovers the build outputs (`kernel.pex`, `loader.bin`, each app, any repos),
- content-hashes them and uploads **only** the blobs the server doesn't already have,
- generates a **Markdown changelog for every application** (written to `build_p68/docs/apps/<NAME>.md`),
- registers a signed release and notifies every connected client live.

The version defaults to `YYYY.MM.DD-n`; pass `--version X` or `--version-from-git` to override.

### From PlusOS Disk Studio

Disk Studio has a **Push Update (admin)…** button. It asks for the server URL and admin password (verified against the server), then a channel and a title, and runs the same signed publish — generating the AI notes and per-app docs along the way. See [Disk Studio & Compiler Pipeline](/wiki/13-plusos-disk-studio-and-compiler).

---

## The AI Update Assistant

Every release gets a short, friendly description and a per-application changelog, written automatically at publish time by a small language model — **LiquidAI LFM2-2.6B**. Users can also chat with the assistant (in the web client, and in `plusupdate`) to ask what changed or how to update.

The assistant runs through whichever backend is configured, tried in order:

1. An OpenAI-compatible endpoint (`OTA_AI_BASE_URL`) — a local **llama.cpp**, **Ollama**, or **LM Studio** server. Best option; set `OTA_AI_MODEL` (default `lfm2-2.6b`).
2. In-process `transformers` + `torch` loading `OTA_AI_MODEL_ID` (default `LiquidAI/LFM2-2.6B`) — opt-in with `OTA_AI_LOCAL=1`.
3. A deterministic template, so publishing never blocks when no model is installed.

---

## The admin control panel

Browse to **https://ota.d0a.net/admin** and log in with the admin password:

- **Overview** — release/artifact/client counts, store size, and live **CPU / RAM / disk** meters, host info, and uptime.
- **Releases** — every release and its artifacts; edit the title/notes, **yank** (hide from clients) or un-yank, delete, and **prune** orphaned blobs to reclaim disk.
- **System** — a live process list (double-click to kill a PID), a **shell console**, and a **Restart Service** button.
- **Audit** — every publish, yank, prune, restart, kill, and shell command, timestamped.
- **Settings** — change the admin password.

> **Security note:** the shell and process-kill features are **OFF by default**. Enable them only with `OTA_ALLOW_SHELL=1`. When enabled, anyone who learns the admin password gets command execution on the host through the public tunnel — use a strong password and leave the shell off unless you need it.

---

## Running the server & going live on ota.d0a.net

The server is at `Servers/otaserver/`. Start it locally:

```cmd
cd Servers\otaserver
run.cmd                         REM creates a venv, installs deps, serves :8799
```

On first run it prints a temporary admin password (or set `OTA_ADMIN_PASSWORD`) and generates the private `.publish_key`.

To publish it at **https://ota.d0a.net** through a Cloudflare Tunnel (no port-forwarding needed — `cloudflared` dials out), run the guided setup once:

```cmd
cd Servers\otaserver\cloudflared
setup_tunnel.cmd                REM install cloudflared, log in, create tunnel + DNS
install_service.cmd             REM (as Administrator) make server + tunnel always-on
curl https://ota.d0a.net/healthz
```

Full details, including the always-on service setup and troubleshooting, are in `Servers/otaserver/cloudflared/SETUP.md` and the server's `README.md`.

### Environment variables

| Variable | Default | Effect |
|:---|:---|:---|
| `OTA_HOST` / `OTA_PORT` | `127.0.0.1` / `8799` | bind address |
| `OTA_PUBLIC_URL` | `https://ota.d0a.net` | shown in the UI; sets the secure-cookie flag |
| `OTA_ADMIN_PASSWORD` | random (printed once) | admin password on first run |
| `OTA_ALLOW_SHELL` | off | enable shell / kill in the control panel |
| `OTA_BRIDGE_PORT` | `8990` | native-client TCP bridge port (0 = off) |
| `OTA_AI_BASE_URL` | — | OpenAI-compatible AI endpoint for the assistant |
| `OTA_AI_LOCAL` | off | allow the in-process LFM2-2.6B backend |

---

## Artifact kinds

| kind | meaning | applied as |
|:---|:---|:---|
| `kernel` | `kernel.pex` boot payload | `hdbuild update --kernel` |
| `loader` | boot block loader | `hdbuild update --loader` |
| `app` | one application | staged into the `bin/` overlay |
| `repo` | a package/app repository (tarball) | staged under `/repos/` |
| `file` | any other file with a target path | staged at that path |

---

## Navigation

- **Previous Chapter**: [← 19. Universal Boot: One Image for Every 680x0 Mac](/wiki/19-universal-boot)
- **Next Chapter**: [21. IKANET: Chat Client & Server →](/wiki/21-ikanet-chat-client)
- **Related**: [15. Package Repository Server & Software Center](/wiki/15-package-repository-and-network-management) · [13. PlusOS Disk Studio & Compiler Pipeline](/wiki/13-plusos-disk-studio-and-compiler) · [09. PlusNET: DaynaPORT TCP/IP Stack](/wiki/09-networking-plusnet)
- **Table of Contents**: [Return to Getting Started](/wiki/01-getting-started)

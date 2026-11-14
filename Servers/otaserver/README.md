# PlusOS OTA — over-the-air update server

A standalone update service for PlusOS. It lets you (the maintainer) publish a
new release on every compile, and lets users pull those updates — new kernel,
apps, and repositories — and apply them to an existing PlusOS disk or BlueSCSI
SD card **without reflashing**, keeping their files.

It is completely independent of IKANET (the chat hub); it has its own server,
database, and web UI.

```
otaserver/
├─ server.py            FastAPI + WebSocket service + chat API (the whole backend)
├─ db.py                SQLite data layer (releases, artifacts, AI docs, chat, audit)
├─ auth.py              publish-key signing + admin password
├─ publish.py           server-side release ingest (+ AI release notes / app docs)
├─ ai.py                Update Assistant — LFM2-2.6B via local runtime (template fallback)
├─ ota_bridge.py        line-protocol TCP bridge for the native 68k client
├─ sysmon.py            system management (RAM/CPU/processes/shell)
├─ publish_cli.py       run on every compile to push a release + per-app docs (stdlib only)
├─ panel/               web UIs — client.html (users) + admin.html (control panel)
├─ updater/             PlusOS Updater — host-side GUI that applies updates
├─ cloudflared/         Cloudflare Tunnel config for ota.d0a.net
├─ run.cmd              one-click Windows launcher
└─ data/               (created at runtime) ota.db + artifact blob store
```

Clients that reach the server:
- **Web client GUI** (`/`) and **host-side PlusOS Updater** (`updater/`) — HTTP/WS.
- **Native PlusOS 68k app** — `apps/network/plusupdate/plusupdate.c`, built by the
  toolchain to `build_p68/apps/plusupdate.pex`. PlusOS has no HTTP/WS stack, so it
  speaks a line protocol over raw TCP to `ota_bridge.py` (port 8990 by default,
  started automatically by `server.py`), exactly like the IKANET chat client.

## The Update Assistant (AI)

`ai.py` generates a friendly message about each release and a Markdown changelog
for every application, at publish time, and powers a chat endpoint. Model:
**LiquidAI LFM2-2.6B**. Backends, tried in order:

1. OpenAI-compatible endpoint at `$OTA_AI_BASE_URL` (llama.cpp server / Ollama /
   LM Studio); set `$OTA_AI_MODEL` (default `lfm2-2.6b`). Best option.
2. In-process `transformers` + `torch` loading `$OTA_AI_MODEL_ID`
   (default `LiquidAI/LFM2-2.6B`) — **opt-in** with `OTA_AI_LOCAL=1` (it can
   download several GB and take minutes).
3. A deterministic template fallback, so publishing never blocks on AI.

Chat: `POST /api/chat {session, message}` → `{reply}`, or the native client's
`CHAT` line command. Per-release docs: `GET /api/release/<id>/docs`.

## Quick start

```cmd
cd otaserver
run.cmd                        REM creates a venv, installs deps, starts on :8799
```

Open:
- **Client update GUI:** http://127.0.0.1:8799/
- **Admin control panel:** http://127.0.0.1:8799/admin

On first run the server prints a temporary admin password (or set
`OTA_ADMIN_PASSWORD` before starting) and generates `otaserver/.publish_key`
— the private secret that authorizes publishing. **Keep `.publish_key`
private; it is git-ignored.**

## Exposing it at ota.d0a.net (Cloudflare)

See `cloudflared/config.yml`. In short:

```cmd
cloudflared tunnel login
cloudflared tunnel create plusos-ota
cloudflared tunnel route dns plusos-ota ota.d0a.net
cloudflared tunnel --config otaserver\cloudflared\config.yml run plusos-ota
```

The tunnel forwards `https://ota.d0a.net` → `127.0.0.1:8799` with WebSocket
support, so nothing needs to be port-forwarded on your machine.

## Publishing on every compile

The publisher hashes your build outputs, uploads only blobs the server doesn't
already have, and registers a signed release:

```cmd
python otaserver\publish_cli.py --server https://ota.d0a.net ^
    --channel stable --machine se --build build_p68 ^
    --title "Desktop speedups" --notes-file RELEASE_NOTES.md
```

Or just build with the Makefile target, which calls it for you:

```cmd
make.cmd ota                   REM = build + publish_cli
```

The version defaults to `YYYY.MM.DD-n`; pass `--version X` or
`--version-from-git` to override. Signing uses `.publish_key`, so **only your
machine can publish** — the key never travels over the wire (it signs an
HMAC challenge bound to a timestamp, so captured requests can't be replayed).

## Applying updates (users)

Users run the **PlusOS Updater**:

```cmd
python otaserver\updater\plusos_updater.py
```

They point it at the OTA server + their PlusOS `.hda` / SD card, see which
releases are newer than what's installed, pick "Select All New" or individual
items, and hit **Update Selected**. The updater downloads the artifacts and
runs `hdbuild update`, so user files (home, documents, Desktop items) and the
disk's Apple SCSI driver are preserved.

The web client GUI (`/`) is the browsable catalog + direct downloads for people
who want to grab artifacts by hand.

## The control panel

`/admin` (admin password) gives you:

- **Overview** — release/artifact/client counts, store size, live CPU / RAM /
  disk meters, host info, server uptime.
- **Releases** — view every release and its artifacts; edit title/notes, yank
  (hide from clients) / un-yank, delete, and prune orphaned blobs to reclaim disk.
- **System** — process list (double-click to kill a PID), a shell console, and
  a "Restart OTA Service" button.
- **Audit** — every publish, yank, prune, restart, kill, and shell command.
- **Settings** — change the admin password.

### Security notes

- **Shell/kill are OFF by default.** Enable with `OTA_ALLOW_SHELL=1`
  (`run.cmd shell`). When enabled, an attacker who learns the admin password
  gets arbitrary command execution on your machine via the public tunnel — use
  a strong password and prefer leaving shell off unless you need it.
- Publishing and the control panel are **independent gates**: the panel password
  cannot publish, and the publish key cannot open the panel.
- Every privileged action is written to the audit log.

## Artifact kinds

| kind         | meaning                                   | applied to |
|--------------|-------------------------------------------|------------|
| `kernel`     | `kernel.pex` boot payload                 | `hdbuild update --kernel` |
| `loader`     | boot block loader                         | `hdbuild update --loader` |
| `app`        | one program                               | staged into `bin/` overlay |
| `apps_store` | raw app store blob                        | (reserved) |
| `repo`       | a package/app repository (tarball or file)| staged under `/repos/` |
| `file`       | any other file with a target `path`       | staged at that path |

## Environment variables

| var                   | default                 | effect |
|-----------------------|-------------------------|--------|
| `OTA_HOST`            | `127.0.0.1`             | bind address |
| `OTA_PORT`            | `8799`                  | bind port |
| `OTA_ADMIN_PASSWORD`  | — (random if unset)     | admin password on first run |
| `OTA_ALLOW_SHELL`     | off                     | enable shell/kill in the panel |
| `OTA_PUBLIC_URL`      | `https://ota.d0a.net`   | shown in UI, sets secure-cookie flag |
| `OTA_SERVER`          | `https://ota.d0a.net`   | default server for publisher + updater |
| `OTA_PUBLISH_KEY`     | — (file preferred)      | publish key if not using `.publish_key` |
| `OTA_BRIDGE_PORT`     | `8990`                  | native-client TCP bridge port (0 = off) |
| `OTA_AI_BASE_URL`     | —                       | OpenAI-compatible AI endpoint |
| `OTA_AI_MODEL`        | `lfm2-2.6b`             | served model name for the HTTP backend |
| `OTA_AI_MODEL_ID`     | `LiquidAI/LFM2-2.6B`    | HF id for the transformers backend |
| `OTA_AI_LOCAL`        | off                     | allow in-process transformers (big download) |

## Publishing from Disk Studio (admin)

Disk Studio has a **Push Update (admin)…** button. It asks for the OTA server
URL and admin password (verified against `/admin/login`), then the channel and a
title, and runs `make.cmd <machine> ota …`, which compiles and publishes a
signed release — generating the AI notes and per-app Markdown docs along the
way. The admin password gates the button; the signing key (`.publish_key`,
present only on your machine) gates the actual publish, so a stranger running
Disk Studio still can't push.

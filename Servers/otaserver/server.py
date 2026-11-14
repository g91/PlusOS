"""PlusOS OTA update server (standalone).

A self-contained FastAPI service that serves PlusOS updates to the host-side
PlusOS Updater over WebSocket (Cloudflare-tunnel friendly), accepts signed
publishes from the maintainer's compile hook, and hosts a password-protected
web control panel with release management + system management.

Run:
    cd otaserver
    pip install -r requirements.txt
    python server.py                      # binds 127.0.0.1:8799 by default
Env:
    OTA_HOST, OTA_PORT                     # bind address
    OTA_ADMIN_PASSWORD                     # set admin password on first run
    OTA_ALLOW_SHELL=1                      # enable shell/kill in the panel
    OTA_PUBLIC_URL                         # e.g. https://ota.d0a.net (shown in UI)

Public (client) surface, works through Cloudflare:
    GET  /                      client update GUI
    WS   /ws/updates            realtime: catalog + live "new release" pushes
    GET  /api/catalog           JSON catalog of releases (optionally ?channel=&machine=)
    GET  /api/latest            latest release for ?channel=&machine=
    GET  /api/blob/{sha256}     download one artifact (content addressed)

Publish (maintainer only, signed):
    GET  /api/have?sha256=...   does the server already have this blob?
    POST /api/blob              upload one artifact blob (signed)
    POST /api/publish           register a release from a manifest (signed)

Admin (cookie session, admin password):
    GET  /admin                 control panel
    POST /admin/login           -> sets cookie
    POST /admin/logout
    GET  /admin/api/*           panel data + actions
"""
from __future__ import annotations

import os
import json
import time
import asyncio
import hashlib
from contextlib import asynccontextmanager
from typing import Optional

from fastapi import (FastAPI, Request, Response, UploadFile, File, Form, Header,
                     HTTPException, WebSocket, WebSocketDisconnect, Query)
from fastapi.responses import FileResponse, JSONResponse, HTMLResponse, PlainTextResponse

import db
import auth
import publish
import sysmon
import ai
import ota_bridge

HERE = os.path.dirname(os.path.abspath(__file__))
PANEL_DIR = os.path.join(HERE, "panel")
PUBLIC_URL = os.environ.get("OTA_PUBLIC_URL", "https://ota.d0a.net")
BRIDGE_PORT = int(os.environ.get("OTA_BRIDGE_PORT", "8990"))
BRIDGE_HOST = os.environ.get("OTA_BRIDGE_HOST", "0.0.0.0")


# ------------------------------------------------------------------ startup

@asynccontextmanager
async def lifespan(app: FastAPI):
    db.get()
    auth.ensure_admin_password()
    auth.ensure_publish_key()
    # Native 68k clients can't do HTTP/WS, so run the line-protocol bridge in the
    # same event loop (disable with OTA_BRIDGE_PORT=0).
    bridge = None
    if BRIDGE_PORT:
        try:
            bridge = await ota_bridge.start(BRIDGE_HOST, BRIDGE_PORT)
            print("PlusOS OTA native bridge on %s:%d" % (BRIDGE_HOST, BRIDGE_PORT))
        except Exception as e:
            print("[warn] could not start native bridge: %s" % e)
    db.audit("system", "startup", "public_url=%s shell=%s bridge=%s ai=%s" % (
        PUBLIC_URL, sysmon.shell_enabled(), BRIDGE_PORT, ai.status()["backend"]))
    yield
    if bridge:
        bridge.close()


app = FastAPI(title="PlusOS OTA", lifespan=lifespan)


# ------------------------------------------------------------------ WS hub (clients)

class UpdateHub:
    def __init__(self):
        self.sockets: set[WebSocket] = set()
        self.lock = asyncio.Lock()

    async def add(self, ws: WebSocket):
        async with self.lock:
            self.sockets.add(ws)

    async def drop(self, ws: WebSocket):
        async with self.lock:
            self.sockets.discard(ws)

    async def push(self, msg: dict):
        text = json.dumps(msg)
        async with self.lock:
            targets = list(self.sockets)
        for ws in targets:
            try:
                await ws.send_text(text)
            except Exception:
                pass


hub = UpdateHub()


def _catalog(channel: Optional[str], machine: Optional[str]) -> dict:
    rows = db.list_releases(channel=channel)
    rels = []
    for r in rows:
        if machine and r["machine"] and r["machine"] not in (machine, "universal"):
            continue
        rels.append(publish.release_dto(r))
    return {"public_url": PUBLIC_URL, "server_time": time.time(),
            "channels": db.stats()["channels"], "releases": rels}


@app.websocket("/ws/updates")
async def ws_updates(ws: WebSocket):
    await ws.accept()
    await hub.add(ws)
    channel = ws.query_params.get("channel") or None
    machine = ws.query_params.get("machine") or None
    try:
        await ws.send_text(json.dumps({"t": "catalog", **_catalog(channel, machine)}))
        while True:
            raw = await ws.receive_text()
            try:
                data = json.loads(raw)
            except Exception:
                continue
            t = data.get("t")
            if t == "ping":
                await ws.send_text(json.dumps({"t": "pong", "n": data.get("n")}))
            elif t == "refresh":
                channel = data.get("channel") or channel
                machine = data.get("machine") or machine
                await ws.send_text(json.dumps({"t": "catalog", **_catalog(channel, machine)}))
    except WebSocketDisconnect:
        pass
    except Exception:
        pass
    finally:
        await hub.drop(ws)


# ------------------------------------------------------------------ public REST

@app.get("/api/catalog")
async def api_catalog(channel: Optional[str] = None, machine: Optional[str] = None):
    return _catalog(channel, machine)


@app.get("/api/latest")
async def api_latest(channel: str = "stable", machine: Optional[str] = None):
    row = db.latest_release(channel, machine)
    if not row:
        raise HTTPException(404, "no release on that channel")
    return publish.release_dto(row)


@app.get("/api/blob/{sha256}")
async def api_blob(sha256: str):
    if not all(c in "0123456789abcdef" for c in sha256.lower()) or len(sha256) != 64:
        raise HTTPException(400, "bad hash")
    path = publish.artifact_path(sha256.lower())
    if not os.path.exists(path):
        raise HTTPException(404, "no such blob")
    return FileResponse(path, media_type="application/octet-stream", filename=sha256)


@app.get("/api/release/{rid}/docs")
async def api_release_docs(rid: int):
    """AI message + per-app Markdown changelogs for one release."""
    row = db.get_release(rid)
    if not row:
        raise HTTPException(404, "no such release")
    return publish.release_dto(row, with_docs=True)


# ------------------------------------------------------------------ AI chat

@app.get("/api/ai/status")
async def api_ai_status():
    return ai.status()


@app.post("/api/chat")
async def api_chat(request: Request):
    """Chat with the Update Assistant. Body: {session, message}. Returns {reply}.
    History is kept per session so the native client can be stateless."""
    data = await request.json()
    session = (data.get("session") or "anon")[:64]
    message = (data.get("message") or "").strip()
    if not message:
        raise HTTPException(400, "empty message")
    hist = [{"role": r["role"], "content": r["content"]} for r in db.chat_history(session, 20)]
    msgs = [{"role": "system", "content": ai.SYSTEM_PROMPT}] + hist + \
           [{"role": "user", "content": message}]
    db.add_chat(session, "user", message)
    reply = await asyncio.to_thread(ai.chat, msgs)
    db.add_chat(session, "assistant", reply)
    return {"reply": reply, "backend": ai.backend()}


# ------------------------------------------------------------------ publish (signed)

def _require_signature(request_body: bytes, headers) -> str:
    """Verify the publish signature over the raw request body. Returns the
    verification mode string or raises 401."""
    ts = headers.get("x-ota-ts")
    sig = headers.get("x-ota-sig")
    bearer = headers.get("x-ota-key")  # bearer fallback only
    if not ts or not sig:
        raise HTTPException(401, "missing X-OTA-TS / X-OTA-Sig")
    try:
        ts_i = int(ts)
    except ValueError:
        raise HTTPException(401, "bad timestamp")
    body_sha = hashlib.sha256(request_body).hexdigest()
    ok, why = auth.verify_publish(ts_i, body_sha, sig, bearer_key=bearer)
    if not ok:
        raise HTTPException(401, "publish rejected: " + why)
    return why


@app.get("/api/have")
async def api_have(sha256: str):
    return {"have": publish.have(sha256.lower())}


@app.post("/api/blob")
async def api_upload_blob(request: Request):
    body = await request.body()
    _require_signature(body, request.headers)
    sha = request.headers.get("x-ota-sha256", "").lower()
    if not sha:
        raise HTTPException(400, "missing X-OTA-Sha256")
    ok, msg = publish.store_blob(sha, body)
    if not ok:
        raise HTTPException(400, msg)
    return {"stored": sha, "size": len(body)}


@app.post("/api/publish")
async def api_publish(request: Request):
    body = await request.body()
    mode = _require_signature(body, request.headers)
    try:
        manifest = json.loads(body.decode("utf-8"))
    except Exception:
        raise HTTPException(400, "manifest is not valid JSON")
    if not manifest.get("version"):
        raise HTTPException(400, "manifest needs a version")
    try:
        result = publish.register_release(manifest, published_by="publisher(%s)" % mode)
    except ValueError as e:
        raise HTTPException(400, str(e))
    # notify every connected client
    await hub.push({"t": "new_release", "channel": result["channel"],
                    "version": result["version"]})
    return result


# ------------------------------------------------------------------ admin auth

COOKIE = "ota_admin"


def _admin_from_request(request: Request):
    token = request.cookies.get(COOKIE)
    if not token:
        return None
    return db.touch_admin_session(token)


def require_admin(request: Request):
    sess = _admin_from_request(request)
    if not sess:
        raise HTTPException(401, "admin login required")
    return sess


@app.post("/admin/login")
async def admin_login(request: Request, response: Response):
    data = await request.json()
    pw = data.get("password", "")
    if not auth.check_admin_password(pw):
        db.audit("admin", "login_fail", _client_ip(request))
        raise HTTPException(401, "wrong password")
    token = db.create_admin_session(_client_ip(request))
    db.audit("admin", "login_ok", _client_ip(request))
    resp = JSONResponse({"ok": True})
    resp.set_cookie(COOKIE, token, httponly=True, samesite="lax",
                    max_age=12 * 3600, secure=PUBLIC_URL.startswith("https"))
    return resp


@app.post("/admin/logout")
async def admin_logout(request: Request):
    token = request.cookies.get(COOKIE)
    if token:
        db.drop_admin_session(token)
    resp = JSONResponse({"ok": True})
    resp.delete_cookie(COOKIE)
    return resp


def _client_ip(request: Request) -> str:
    # Behind Cloudflare the real client is in CF-Connecting-IP.
    return (request.headers.get("cf-connecting-ip")
            or request.headers.get("x-forwarded-for", "").split(",")[0].strip()
            or (request.client.host if request.client else "?"))


# ------------------------------------------------------------------ admin API

@app.get("/admin/api/whoami")
async def admin_whoami(request: Request):
    sess = _admin_from_request(request)
    return {"logged_in": bool(sess), "shell_enabled": sysmon.shell_enabled(),
            "public_url": PUBLIC_URL}


@app.get("/admin/api/overview")
async def admin_overview(request: Request):
    require_admin(request)
    return {"stats": db.stats(), "metrics": sysmon.metrics(),
            "connected_clients": len(hub.sockets), "ai": ai.status()}


@app.get("/admin/api/releases")
async def admin_releases(request: Request, channel: Optional[str] = None):
    require_admin(request)
    rows = db.list_releases(channel=channel, include_yanked=True)
    return {"releases": [publish.release_dto(r) for r in rows]}


@app.post("/admin/api/release/{rid}/yank")
async def admin_yank(request: Request, rid: int):
    require_admin(request)
    data = await request.json()
    yank = bool(data.get("yank", True))
    db.set_yanked(rid, yank)
    db.audit("admin", "yank" if yank else "unyank", "release=%d" % rid)
    await hub.push({"t": "catalog_changed"})
    return {"ok": True}


@app.post("/admin/api/release/{rid}/notes")
async def admin_edit_notes(request: Request, rid: int):
    require_admin(request)
    data = await request.json()
    row = db.get_release(rid)
    if not row:
        raise HTTPException(404, "no such release")
    db.create_release(row["channel"], row["version"], row["machine"],
                      data.get("title", row["title"]), data.get("notes", row["notes"]),
                      row["published_by"])
    db.audit("admin", "edit_notes", "release=%d" % rid)
    return {"ok": True}


@app.post("/admin/api/release/{rid}/delete")
async def admin_delete(request: Request, rid: int):
    require_admin(request)
    db.delete_release(rid)
    db.audit("admin", "delete_release", "release=%d" % rid)
    await hub.push({"t": "catalog_changed"})
    return {"ok": True}


@app.post("/admin/api/prune")
async def admin_prune(request: Request):
    """Delete orphaned blobs (no artifact references them) to reclaim disk."""
    require_admin(request)
    freed = 0
    for h in db.orphan_hashes():
        p = publish.artifact_path(h)
        try:
            freed += os.path.getsize(p)
            os.remove(p)
        except OSError:
            pass
    db.audit("admin", "prune", "freed=%d bytes" % freed)
    return {"ok": True, "freed_bytes": freed}


@app.get("/admin/api/audit")
async def admin_audit(request: Request, limit: int = 120):
    require_admin(request)
    return {"audit": [dict(r) for r in db.recent_audit(limit)]}


@app.post("/admin/api/password")
async def admin_change_password(request: Request):
    require_admin(request)
    data = await request.json()
    old, new = data.get("old", ""), data.get("new", "")
    if not auth.check_admin_password(old):
        raise HTTPException(401, "current password is wrong")
    if len(new) < 6:
        raise HTTPException(400, "new password too short (min 6)")
    auth.set_admin_password(new)
    db.audit("admin", "change_password", "")
    return {"ok": True}


# ---- system management ----

@app.get("/admin/api/processes")
async def admin_processes(request: Request, limit: int = 40):
    require_admin(request)
    return {"processes": sysmon.processes(limit)}


@app.post("/admin/api/kill")
async def admin_kill(request: Request):
    require_admin(request)
    data = await request.json()
    ok, msg = sysmon.kill(int(data.get("pid", -1)))
    db.audit("admin", "kill", "pid=%s ok=%s %s" % (data.get("pid"), ok, msg))
    return {"ok": ok, "msg": msg}


@app.post("/admin/api/shell")
async def admin_shell(request: Request):
    require_admin(request)
    data = await request.json()
    cmd = data.get("cmd", "")
    db.audit("admin", "shell", cmd[:500])
    return sysmon.run_shell(cmd, timeout=int(data.get("timeout", 30)))


@app.post("/admin/api/restart")
async def admin_restart(request: Request):
    require_admin(request)
    db.audit("admin", "restart", "")
    # Reply first, then re-exec shortly after.
    async def _later():
        await asyncio.sleep(0.5)
        sysmon.restart_service()
    asyncio.get_event_loop().create_task(_later())
    return {"ok": True, "msg": "restarting"}


# ------------------------------------------------------------------ pages

@app.get("/", response_class=HTMLResponse)
async def page_client():
    return _serve(os.path.join(PANEL_DIR, "client.html"))


@app.get("/admin", response_class=HTMLResponse)
async def page_admin():
    return _serve(os.path.join(PANEL_DIR, "admin.html"))


@app.get("/panel/{fname}")
async def page_asset(fname: str):
    # serve css/js next to the pages
    if "/" in fname or "\\" in fname or ".." in fname:
        raise HTTPException(404)
    path = os.path.join(PANEL_DIR, fname)
    if not os.path.exists(path):
        raise HTTPException(404)
    mime = ("text/css" if fname.endswith(".css")
            else "application/javascript" if fname.endswith(".js")
            else "image/png" if fname.endswith(".png")
            else "application/octet-stream")
    return FileResponse(path, media_type=mime)


@app.get("/healthz", response_class=PlainTextResponse)
async def healthz():
    return "ok"


def _serve(path: str) -> HTMLResponse:
    if not os.path.exists(path):
        return HTMLResponse("<h1>panel file missing: %s</h1>" % os.path.basename(path), 500)
    with open(path, "r", encoding="utf-8") as fh:
        return HTMLResponse(fh.read())


if __name__ == "__main__":
    import uvicorn
    host = os.environ.get("OTA_HOST", "127.0.0.1")
    port = int(os.environ.get("OTA_PORT", "8799"))
    print("PlusOS OTA server on http://%s:%d  (public: %s)" % (host, port, PUBLIC_URL))
    uvicorn.run(app, host=host, port=port, ws_ping_interval=20, ws_ping_timeout=20)

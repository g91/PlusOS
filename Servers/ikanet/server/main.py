"""IKANET server — FastAPI + WebSocket chat hub.

Run:  python -m uvicorn main:app --host 0.0.0.0 --port 8787
(or just `python main.py`)

Endpoints
---------
POST /api/join          {username, password, icon?}  -> {token, user}
POST /api/login         {username, password}          -> {token, user}
POST /api/logout        (Bearer token)
POST /api/upload        multipart file (Bearer token) -> {file}
GET  /api/file/{id}                                    -> raw bytes (download squid)
GET  /api/thumb/{id}                                   -> raw bytes (preview window)
WS   /ws?token=...      realtime chat
GET  /                  serves the client
"""
from __future__ import annotations

import io
import os
import re
import json
import time
import random
import asyncio
import secrets
import struct
from typing import Optional

from fastapi import FastAPI, UploadFile, File, Form, Header, HTTPException, WebSocket, WebSocketDisconnect
from fastapi.responses import FileResponse, JSONResponse, Response
from fastapi.staticfiles import StaticFiles
import bcrypt as _bcrypt

import db


class bcrypt:  # tiny passlib-free shim
    @staticmethod
    def hash(pw: str) -> str:
        return _bcrypt.hashpw(pw.encode("utf-8")[:72], _bcrypt.gensalt()).decode("ascii")

    @staticmethod
    def verify(pw: str, hashed: str) -> bool:
        try:
            return _bcrypt.checkpw(pw.encode("utf-8")[:72], hashed.encode("ascii"))
        except Exception:
            return False

app = FastAPI(title="IKANET")

CLIENT_DIR = os.path.normpath(os.path.join(os.path.dirname(__file__), "..", "client"))

QUOTES = [
    "LONG LIVE THE APPLE III",
    "640K OUGHT TO BE ENOUGH",
    "INSERT DISK 2 OF 47",
    "ABORT, RETRY, FAIL?",
    "THE CAKE IS A FLOPPY",
    "RECONNECTING TO THE INK CLOUD...",
    "SOSUMI",
    "IT IS NOW SAFE TO SWIM",
    "PRESS ANY TENTACLE TO CONTINUE",
    "MORE RAM = MORE SQUID",
]

USERNAME_RE = re.compile(r"^[A-Za-z0-9_]{2,20}$")

# ----------------------------------------------------------------- helpers

def auth_user(authorization: Optional[str]):
    if not authorization or not authorization.lower().startswith("bearer "):
        raise HTTPException(401, "missing token")
    token = authorization.split(" ", 1)[1].strip()
    user = db.user_for_token(token)
    if not user:
        raise HTTPException(401, "bad token")
    return user, token


def public_user(row) -> dict:
    return {
        "id": row["id"],
        "username": row["username"],
        "icon": row["icon"],
        "appear_offline": bool(row["appear_offline"]),
    }


def png_dimensions(data: bytes):
    """Cheap width/height sniff for png/gif/bmp without Pillow."""
    try:
        if data[:8] == b"\x89PNG\r\n\x1a\n":
            w, h = struct.unpack(">II", data[16:24])
            return int(w), int(h)
        if data[:6] in (b"GIF87a", b"GIF89a"):
            w, h = struct.unpack("<HH", data[6:10])
            return int(w), int(h)
        if data[:2] == b"BM":
            w, h = struct.unpack("<ii", data[18:26])
            return abs(w), abs(h)
        if data[:2] == b"\xff\xd8":  # jpeg — walk segments
            i = 2
            while i < len(data) - 9:
                if data[i] != 0xFF:
                    i += 1
                    continue
                marker = data[i + 1]
                if marker in (0xC0, 0xC1, 0xC2, 0xC3):
                    h, w = struct.unpack(">HH", data[i + 5:i + 9])
                    return int(w), int(h)
                seg = struct.unpack(">H", data[i + 2:i + 4])[0]
                i += 2 + seg
    except Exception:
        pass
    return None, None


# ----------------------------------------------------------------- auth API

@app.post("/api/join")
async def join(payload: dict):
    username = (payload.get("username") or "").strip()
    password = payload.get("password") or ""
    icon = payload.get("icon")
    if not USERNAME_RE.match(username):
        raise HTTPException(400, "Username must be 2-20 letters, digits or _")
    if len(password) < 3:
        raise HTTPException(400, "Password too short")
    if db.get_user_by_name(username):
        raise HTTPException(409, "That name is taken, m8")
    uid = db.create_user(username, bcrypt.hash(password), icon)
    token = db.create_session(uid)
    return {"token": token, "user": public_user(db.get_user(uid))}


@app.post("/api/login")
async def login(payload: dict):
    username = (payload.get("username") or "").strip()
    password = payload.get("password") or ""
    row = db.get_user_by_name(username)
    if not row or not bcrypt.verify(password, row["pass_hash"]):
        raise HTTPException(401, "Wrong user or pass")
    token = db.create_session(row["id"])
    return {"token": token, "user": public_user(row)}


@app.post("/api/logout")
async def logout(authorization: Optional[str] = Header(None)):
    _, token = auth_user(authorization)
    db.drop_session(token)
    return {"ok": True}


# ----------------------------------------------------------------- files

@app.post("/api/upload")
async def upload(file: UploadFile = File(...), authorization: Optional[str] = Header(None)):
    user, _ = auth_user(authorization)
    data = await file.read()
    if len(data) > 8 * 1024 * 1024:
        raise HTTPException(413, "File too big (8MB max)")
    disk_name = f"{int(time.time())}_{secrets.token_hex(6)}_{file.filename}"
    disk_name = re.sub(r"[^A-Za-z0-9._-]", "_", disk_name)[:160]
    path = os.path.join(db.UPLOAD_DIR, disk_name)
    with open(path, "wb") as fh:
        fh.write(data)
    is_image = (file.content_type or "").startswith("image/")
    w, h = png_dimensions(data) if is_image else (None, None)
    fid = db.add_file(
        user["id"], disk_name, file.filename or "untitled",
        file.content_type or "application/octet-stream",
        len(data), w, h, is_image,
    )
    return {"file": file_info(db.get_file(fid))}


def file_info(row) -> dict:
    return {
        "id": row["id"],
        "name": row["orig_name"],
        "mime": row["mime"],
        "size": row["size"],
        "width": row["width"],
        "height": row["height"],
        "is_image": bool(row["is_image"]),
    }


@app.get("/api/file/{fid}")
async def get_file_raw(fid: int):
    row = db.get_file(fid)
    if not row:
        raise HTTPException(404, "no such file")
    path = os.path.join(db.UPLOAD_DIR, row["disk_name"])
    if not os.path.exists(path):
        raise HTTPException(404, "file gone")
    return FileResponse(path, media_type=row["mime"], filename=row["orig_name"])


@app.get("/api/thumb/{fid}")
async def get_thumb(fid: int):
    # The client scales in CSS; we just serve the image inline.
    row = db.get_file(fid)
    if not row or not row["is_image"]:
        raise HTTPException(404, "no image")
    path = os.path.join(db.UPLOAD_DIR, row["disk_name"])
    if not os.path.exists(path):
        raise HTTPException(404, "file gone")
    return FileResponse(path, media_type=row["mime"])


# ----------------------------------------------------------------- WS hub

class Hub:
    def __init__(self):
        # channel_id -> set[ (ws, username) ]
        self.by_channel: dict[int, set] = {}
        # username(lower) -> ws   (one active socket per user, last wins)
        self.by_user: dict[str, WebSocket] = {}
        self.lock = asyncio.Lock()

    async def join(self, cid: int, ws: WebSocket, username: str):
        async with self.lock:
            self.by_channel.setdefault(cid, set()).add((ws, username))

    async def leave_all(self, ws: WebSocket):
        async with self.lock:
            for members in self.by_channel.values():
                members.discard(next((m for m in members if m[0] is ws), None))
            dead = [u for u, w in self.by_user.items() if w is ws]
            for u in dead:
                del self.by_user[u]

    async def broadcast(self, cid: int, msg: dict):
        members = list(self.by_channel.get(cid, ()))
        text = json.dumps(msg)
        for ws, _ in members:
            try:
                await ws.send_text(text)
            except Exception:
                pass

    def presence(self, cid: int) -> list[str]:
        return sorted({u for _, u in self.by_channel.get(cid, ())})


hub = Hub()


async def send_channel_history(ws: WebSocket, cid: int, split: bool = False, limit: int = 60):
    """Send a room's recent messages.

    Browsers get one "history" frame. Small clients (the native 68k IKANET app)
    ask for split=True: a history_begin marker, one ordinary "msg" frame per
    message, then history_end. A 68000 cannot buffer a ~25 KB history frame, so
    every frame it receives stays well under 1 KB.
    """
    limit = max(1, min(int(limit or 60), 200))
    rows = db.recent_messages(cid, limit)
    if not split:
        out = [serialize_message(r) for r in rows]
        await ws.send_text(json.dumps({"t": "history", "channel_id": cid, "messages": out}))
        return
    await ws.send_text(json.dumps({"t": "history_begin", "channel_id": cid, "count": len(rows)}))
    for r in rows:
        await ws.send_text(json.dumps({"t": "msg", "hist": True, **serialize_message(r)}))
    await ws.send_text(json.dumps({"t": "history_end", "channel_id": cid}))


def serialize_message(r) -> dict:
    user = db.get_user(r["user_id"]) if r["user_id"] else None
    f = db.get_file(r["file_id"]) if r["file_id"] else None
    return {
        "id": r["id"],
        "channel_id": r["channel_id"],
        "user": user["username"] if user else None,
        "icon": user["icon"] if user else None,
        "system": r["user_id"] is None,
        "body": r["body"],
        "file": file_info(f) if f else None,
        "ts": r["created_at"],
    }


async def _authenticate_ws(ws: WebSocket):
    """Return a user row, or None. Accepts either a ?token=... query param
    (web client, which logged in over HTTP first) or a first JSON frame
    {"t":"auth","mode":"login"|"join","user":...,"pass":...} so the native
    68k client can authenticate entirely over the WebSocket — one port, one
    protocol, Cloudflare-friendly."""
    token = ws.query_params.get("token", "")
    if token:
        user = db.user_for_token(token)
        if user:
            await ws.accept()
            return user
        await ws.close(code=4001)
        return None

    # No token: accept and wait for an inline auth frame (native client path).
    await ws.accept()
    try:
        raw = await asyncio.wait_for(ws.receive_text(), timeout=30)
        data = json.loads(raw)
    except WebSocketDisconnect:
        return None                      # client hung up first: nothing to close
    except Exception:
        try:
            await ws.close(code=4002)
        except Exception:
            pass
        return None
    if data.get("t") != "auth":
        await ws.send_text(json.dumps({"t": "error", "msg": "auth required"}))
        await ws.close(code=4002)
        return None
    username = (data.get("user") or "").strip()
    password = data.get("pass") or ""
    mode = data.get("mode") or "login"
    icon = data.get("icon")
    # a valid icon is exactly 256 chars of '0'/'1' (16x16 mono)
    if not (isinstance(icon, str) and len(icon) == 256 and set(icon) <= {"0", "1"}):
        icon = None
    row = db.get_user_by_name(username)
    if mode == "join":
        if not USERNAME_RE.match(username):
            await ws.send_text(json.dumps({"t": "error", "msg": "Bad username"})); await ws.close(); return None
        if len(password) < 3:
            await ws.send_text(json.dumps({"t": "error", "msg": "Password too short"})); await ws.close(); return None
        if row:
            await ws.send_text(json.dumps({"t": "error", "msg": "That name is taken, m8"})); await ws.close(); return None
        uid = db.create_user(username, bcrypt.hash(password), icon)
        row = db.get_user(uid)
    else:
        if not row or not bcrypt.verify(password, row["pass_hash"]):
            await ws.send_text(json.dumps({"t": "error", "msg": "Wrong user or pass"})); await ws.close(); return None
    db.create_session(row["id"])  # keeps parity; token not needed over this socket
    return row


@app.websocket("/ws")
async def ws_endpoint(ws: WebSocket):
    user = await _authenticate_ws(ws)
    if not user:
        return
    uname = user["username"]
    async with hub.lock:
        hub.by_user[uname.lower()] = ws

    # Join the user's saved channels so presence & broadcast work.
    saved = db.saved_channels(user["id"])
    for ch in saved:
        await hub.join(ch["id"], ws, uname)

    # Tell the client who they are + their channel list.
    await ws.send_text(json.dumps({
        "t": "welcome",
        "user": public_user(user),
        "channels": [channel_dto(c) for c in saved],
        "quote": random.choice(QUOTES),
    }))

    try:
        while True:
            raw = await ws.receive_text()
            await handle_ws_message(ws, user, json.loads(raw))
    except WebSocketDisconnect:
        pass
    except Exception:
        pass
    finally:
        await hub.leave_all(ws)
        # presence update to everyone who shared a channel
        for ch in db.saved_channels(user["id"]):
            await hub.broadcast(ch["id"], {
                "t": "presence", "channel_id": ch["id"],
                "users": hub.presence(ch["id"]),
            })


def channel_dto(c) -> dict:
    return {"id": c["id"], "name": c["name"], "is_dm": bool(c["is_dm"])}


async def handle_ws_message(ws: WebSocket, user, data: dict):
    t = data.get("t")

    if t == "ping":
        # client measures RTT off this echo
        await ws.send_text(json.dumps({"t": "pong", "n": data.get("n")}))
        return

    if t == "subscribe":
        cid = int(data["channel_id"])
        await hub.join(cid, ws, user["username"])
        await send_channel_history(ws, cid, split=bool(data.get("split")),
                                   limit=data.get("limit") or 60)
        await hub.broadcast(cid, {
            "t": "presence", "channel_id": cid, "users": hub.presence(cid),
        })
        return

    if t == "say":
        cid = int(data["channel_id"])
        body = (data.get("body") or "").strip()
        file_id = data.get("file_id")
        if not body and not file_id:
            return
        row = db.add_message(cid, user["id"], body or None, file_id)
        await hub.broadcast(cid, {"t": "msg", **serialize_message(row)})
        return

    if t == "open_dm":
        other = db.get_user_by_name(data.get("username", ""))
        if not other:
            await ws.send_text(json.dumps({"t": "error", "msg": "No such user"}))
            return
        name = db.dm_channel_name(user["username"], other["username"])
        ch = db.get_or_create_channel(name, is_dm=True)
        db.join_channel(user["id"], ch["id"])
        db.join_channel(other["id"], ch["id"])
        await hub.join(ch["id"], ws, user["username"])
        await ws.send_text(json.dumps({
            "t": "dm_opened",
            "channel": {**channel_dto(ch), "with": other["username"]},
        }))
        if not data.get("split"):                 # split clients re-subscribe themselves
            await send_channel_history(ws, ch["id"])
        return

    if t == "add_channel":
        name = (data.get("name") or "").strip()
        if not name.startswith("#"):
            name = "#" + name
        name = re.sub(r"[^#A-Za-z0-9_-]", "", name)[:30]
        if len(name) < 2:
            return
        ch = db.get_or_create_channel(name)
        db.join_channel(user["id"], ch["id"], saved=True)
        await hub.join(ch["id"], ws, user["username"])
        await ws.send_text(json.dumps({"t": "channel_added", "channel": channel_dto(ch)}))
        if not data.get("split"):                 # split clients re-subscribe themselves
            await send_channel_history(ws, ch["id"])
        return

    if t == "leave_channel":
        cid = int(data["channel_id"])
        db.leave_channel(user["id"], cid)
        await ws.send_text(json.dumps({"t": "channel_left", "channel_id": cid}))
        return

    if t == "list_channels":
        await ws.send_text(json.dumps({
            "t": "channel_list",
            "public": [channel_dto(c) for c in db.public_channels()],
            "saved": [channel_dto(c) for c in db.saved_channels(user["id"])],
        }))
        return

    if t == "set_icon":
        icon = data.get("icon", "")
        # enforce a valid 16x16 mono bitmap (256 chars of '0'/'1')
        if not (isinstance(icon, str) and len(icon) == 256 and set(icon) <= {"0", "1"}):
            await ws.send_text(json.dumps({"t": "error", "msg": "Icon must be 16x16"}))
            return
        db.set_icon(user["id"], icon)
        await ws.send_text(json.dumps({"t": "icon_set", "icon": icon}))
        return

    if t == "appear_offline":
        db.set_appear_offline(user["id"], bool(data.get("value")))
        return


# ----------------------------------------------------------------- static

# Note: the native 68k client now speaks plain WebSocket to /ws (see
# _authenticate_ws), so everything rides one port behind Cloudflare (443 -> 8787).
# The old raw-TCP bridge (tcp_bridge.py, port 8788) is retired because Cloudflare's
# proxy does not forward arbitrary TCP. The module is kept for direct-LAN use only.

app.mount("/", StaticFiles(directory=CLIENT_DIR, html=True), name="client")


if __name__ == "__main__":
    import uvicorn
    uvicorn.run(app, host="0.0.0.0", port=8787)

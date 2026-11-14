"""IKANET plain-TCP bridge for the native PlusOS 68k client.

The 68k app can't easily do WebSocket framing, so it speaks a tiny line
protocol over raw TCP. This bridge shares the same SQLite DB and the same
in-memory Hub as the web client, so native users and browser users chat
together in the same rooms.

Wire protocol: newline-terminated lines, fields separated by 0x01.

  client -> server
    JOIN\x01user\x01pass        LOGIN\x01user\x01pass
    CHAN\x01#name               SAY\x01text
    DM\x01user                  WHO
    PING                        QUIT

  server -> client
    OK\x01token                 ERR\x01message
    MSG\x01user\x01text         SYS\x01text
    QUOTE\x01text               PONG
    CHAN\x01#name
"""
from __future__ import annotations

import asyncio
import random

SEP = "\x01"


class TcpClient:
    def __init__(self, reader, writer, srv):
        self.reader = reader
        self.writer = writer
        self.srv = srv          # the shared server module (main)
        self.user = None        # sqlite row
        self.token = None
        self.channel_id = None

    async def send(self, *fields):
        line = SEP.join(str(f) for f in fields) + "\n"
        try:
            self.writer.write(line.encode("utf-8", "replace"))
            await self.writer.drain()
        except Exception:
            pass


# Native TCP sockets registered per channel so broadcast() can reach them.
# channel_id -> set[TcpClient]
_by_channel: dict[int, set] = {}


def _register(cid: int, c: TcpClient):
    _by_channel.setdefault(cid, set()).add(c)


def _unregister(c: TcpClient):
    for members in _by_channel.values():
        members.discard(c)


async def broadcast_native(cid: int, user: str | None, body: str, system: bool):
    """Called by the web hub when a message lands, so TCP clients see it too."""
    for c in list(_by_channel.get(cid, ())):
        if system:
            await c.send("SYS", body)
        else:
            await c.send("MSG", user or "?", body or "")


def make_handler(main):
    """main is the imported server module (db access + hub + helpers)."""
    db = main.db

    async def handle(reader, writer):
        c = TcpClient(reader, writer, main)
        peer = writer.get_extra_info("peername")
        try:
            while True:
                raw = await reader.readline()
                if not raw:
                    break
                line = raw.decode("utf-8", "replace").rstrip("\r\n")
                if not line:
                    continue
                parts = line.split(SEP)
                cmd = parts[0].upper()

                if cmd in ("JOIN", "LOGIN"):
                    user = parts[1].strip() if len(parts) > 1 else ""
                    pw = parts[2] if len(parts) > 2 else ""
                    row = db.get_user_by_name(user)
                    if cmd == "JOIN":
                        if not main.USERNAME_RE.match(user):
                            await c.send("ERR", "Bad username"); continue
                        if len(pw) < 3:
                            await c.send("ERR", "Password too short"); continue
                        if row:
                            await c.send("ERR", "That name is taken, m8"); continue
                        uid = db.create_user(user, main.bcrypt.hash(pw), None)
                        row = db.get_user(uid)
                    else:
                        if not row or not main.bcrypt.verify(pw, row["pass_hash"]):
                            await c.send("ERR", "Wrong user or pass"); continue
                    c.user = row
                    c.token = db.create_session(row["id"])
                    await c.send("OK", c.token)
                    await c.send("QUOTE", random.choice(main.QUOTES))
                    continue

                if c.user is None:
                    await c.send("ERR", "Not logged in"); continue

                if cmd == "CHAN":
                    name = (parts[1] if len(parts) > 1 else "#general").strip()
                    if not name.startswith("#"):
                        name = "#" + name
                    ch = db.get_or_create_channel(name)
                    db.join_channel(c.user["id"], ch["id"], saved=True)
                    _unregister(c)
                    _register(ch["id"], c)
                    c.channel_id = ch["id"]
                    await c.send("CHAN", ch["name"])
                    for r in db.recent_messages(ch["id"], 40):
                        await _emit_row(c, r, db)

                elif cmd == "DM":
                    other = db.get_user_by_name(parts[1] if len(parts) > 1 else "")
                    if not other:
                        await c.send("ERR", "No such user"); continue
                    name = db.dm_channel_name(c.user["username"], other["username"])
                    ch = db.get_or_create_channel(name, is_dm=True)
                    db.join_channel(c.user["id"], ch["id"])
                    db.join_channel(other["id"], ch["id"])
                    _unregister(c); _register(ch["id"], c); c.channel_id = ch["id"]
                    await c.send("CHAN", "@" + other["username"])
                    for r in db.recent_messages(ch["id"], 40):
                        await _emit_row(c, r, db)

                elif cmd == "SAY":
                    if c.channel_id is None:
                        await c.send("ERR", "No channel"); continue
                    body = (parts[1] if len(parts) > 1 else "").strip()
                    if not body:
                        continue
                    row = db.add_message(c.channel_id, c.user["id"], body, None)
                    # fan out to both web (hub) and native clients
                    msg = main.serialize_message(row)
                    await main.hub.broadcast(c.channel_id, {"t": "msg", **msg})
                    await broadcast_native(c.channel_id, c.user["username"], body, False)

                elif cmd == "WHO":
                    names = main.hub.presence(c.channel_id) if c.channel_id else []
                    await c.send("SYS", "here: " + (", ".join(names) or "(just you)"))

                elif cmd == "PING":
                    await c.send("PONG")

                elif cmd == "QUIT":
                    break
        except (asyncio.CancelledError, ConnectionResetError):
            pass
        except Exception:
            pass
        finally:
            _unregister(c)
            if c.token:
                db.drop_session(c.token)
            try:
                writer.close()
            except Exception:
                pass

    return handle


async def _emit_row(c: TcpClient, r, db):
    if r["user_id"] is None:
        await c.send("SYS", r["body"] or "")
    else:
        u = db.get_user(r["user_id"])
        body = r["body"] or ""
        f = db.get_file(r["file_id"]) if r["file_id"] else None
        if f:
            body = (body + " " if body else "") + f"[file: {f['orig_name']}]"
        await c.send("MSG", u["username"] if u else "?", body)


async def start(main, host="0.0.0.0", port=8788):
    handler = make_handler(main)
    server = await asyncio.start_server(handler, host, port)
    return server

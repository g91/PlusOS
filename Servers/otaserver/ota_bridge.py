"""OTA line-protocol bridge for the native PlusOS 68k update app.

PlusOS has no HTTP or WebSocket stack, so — exactly like the IKANET chat client
— the native "PlusUpdate" app speaks a tiny newline-terminated line protocol
over raw TCP (fields separated by 0x01). This bridge serves the OTA catalog,
AI-written update messages, per-app changelog docs, and the AI chat, reading
the same SQLite DB the HTTP server uses.

Run it alongside server.py (or let server.py launch it — see --embedded):

    python ota_bridge.py                 # listens on 0.0.0.0:8799 -> no, that's HTTP
    python ota_bridge.py --port 8990

Wire protocol (client -> server):
    HELLO\x01<machine>\x01<channel>      announce + get a greeting
    LIST                                 list releases for the announced channel/machine
    INFO\x01<release_id>                  release detail + AI message
    DOC\x01<release_id>\x01<appname>      one app's Markdown changelog (chunked)
    CHAT\x01<text>                        ask the AI assistant
    PING                                 -> PONG

Server -> client:
    HI\x01<greeting>
    REL\x01id\x01channel\x01version\x01machine\x01title\x01bytes\x01nitems\x01new?
    END                                   end of a REL list
    MSG\x01<ai message text>              (INFO)
    ITEM\x01name\x01kind\x01size\x01sha    (INFO, one per artifact)
    DOCLINE\x01<text>                      (DOC, one per line of markdown)
    DOCEND
    AI\x01<reply text>                     (CHAT)
    ERR\x01<message>
    PONG
Long text is sent as multiple lines where noted; newlines inside a field are
replaced with \x02 so the client can restore them.
"""
from __future__ import annotations

import os
import sys
import json
import asyncio
import argparse

import db
import publish
import ai

SEP = "\x01"
NL = "\x02"          # stand-in for newline inside a single protocol field


def _enc(s: str) -> str:
    return (s or "").replace("\r", "").replace("\n", NL)


class Conn:
    def __init__(self, reader, writer):
        self.reader = reader
        self.writer = writer
        self.machine = None
        self.channel = "stable"
        self.session = "native-%s" % id(self)

    async def send(self, *fields):
        line = SEP.join(_enc(str(f)) for f in fields) + "\n"
        try:
            self.writer.write(line.encode("utf-8", "replace"))
            await self.writer.drain()
        except Exception:
            pass


async def _handle(reader, writer):
    c = Conn(reader, writer)
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

            if cmd == "HELLO":
                c.machine = (parts[1] if len(parts) > 1 else "") or None
                c.channel = (parts[2] if len(parts) > 2 else "stable") or "stable"
                await c.send("HI", "PlusOS Update Service — channel %s, machine %s"
                             % (c.channel, c.machine or "any"))

            elif cmd == "PING":
                await c.send("PONG")

            elif cmd == "LIST":
                rows = db.list_releases(channel=c.channel)
                for r in rows:
                    if c.machine and r["machine"] and r["machine"] not in (c.machine, "universal"):
                        continue
                    dto = publish.release_dto(r)
                    await c.send("REL", r["id"], r["channel"], r["version"],
                                 r["machine"] or "", dto["title"] or "",
                                 dto["total_bytes"], len(dto["artifacts"]), 1)
                await c.send("END")

            elif cmd == "INFO":
                rid = int(parts[1]) if len(parts) > 1 and parts[1].isdigit() else -1
                row = db.get_release(rid)
                if not row:
                    await c.send("ERR", "no such release"); continue
                dto = publish.release_dto(row)
                await c.send("MSG", dto.get("ai_message") or dto.get("notes") or "")
                for a in dto["artifacts"]:
                    await c.send("ITEM", a["name"], a["kind"], a["size"], a["sha256"])
                await c.send("END")

            elif cmd == "DOC":
                rid = int(parts[1]) if len(parts) > 1 and parts[1].isdigit() else -1
                name = parts[2] if len(parts) > 2 else ""
                docs = {r["name"]: r["body"] for r in db.release_app_docs(rid)}
                body = docs.get(name)
                if body is None:
                    # fall back to the release message
                    body = db.release_ai_message(rid) or "(no document)"
                for ln in body.splitlines() or [""]:
                    await c.send("DOCLINE", ln)
                await c.send("DOCEND")

            elif cmd == "CHAT":
                text = (parts[1] if len(parts) > 1 else "").strip()
                if not text:
                    await c.send("ERR", "empty"); continue
                hist = [{"role": r["role"], "content": r["content"]}
                        for r in db.chat_history(c.session, 20)]
                msgs = [{"role": "system", "content": ai.SYSTEM_PROMPT}] + hist + \
                       [{"role": "user", "content": text}]
                db.add_chat(c.session, "user", text)
                reply = await asyncio.to_thread(ai.chat, msgs)
                db.add_chat(c.session, "assistant", reply)
                await c.send("AI", reply)

            elif cmd == "QUIT":
                break
    except (asyncio.CancelledError, ConnectionResetError):
        pass
    except Exception:
        pass
    finally:
        try:
            writer.close()
        except Exception:
            pass


async def start(host="0.0.0.0", port=8990):
    db.get()
    server = await asyncio.start_server(_handle, host, port)
    return server


async def _main(host, port):
    srv = await start(host, port)
    print("PlusOS OTA bridge (native 68k line protocol) on %s:%d" % (host, port))
    async with srv:
        await srv.serve_forever()


if __name__ == "__main__":
    ap = argparse.ArgumentParser(description="OTA line-protocol bridge for the native PlusOS client")
    ap.add_argument("--host", default=os.environ.get("OTA_BRIDGE_HOST", "0.0.0.0"))
    ap.add_argument("--port", type=int, default=int(os.environ.get("OTA_BRIDGE_PORT", "8990")))
    a = ap.parse_args()
    try:
        asyncio.run(_main(a.host, a.port))
    except KeyboardInterrupt:
        pass

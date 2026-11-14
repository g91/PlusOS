"""OTA update server — SQLite data layer.

Tracks releases (one per compile/publish), the artifacts inside each release
(kernel, app store, individual apps, repos), admin control-panel sessions, and
an audit log of every privileged action.

A single sqlite file lives in ./data/ota.db. Artifact *bytes* live on disk in
./data/artifacts/<sha256> (content addressed, so re-publishing an unchanged
file costs nothing); the DB only stores metadata + the hash.
"""
from __future__ import annotations

import os
import sqlite3
import time
import json
import secrets
import threading
from typing import Optional

DATA_DIR = os.path.normpath(os.path.join(os.path.dirname(__file__), "data"))
ARTIFACT_DIR = os.path.join(DATA_DIR, "artifacts")
DB_PATH = os.path.join(DATA_DIR, "ota.db")

os.makedirs(ARTIFACT_DIR, exist_ok=True)

_conn: Optional[sqlite3.Connection] = None
_lock = threading.RLock()

SCHEMA = """
CREATE TABLE IF NOT EXISTS releases (
    id          INTEGER PRIMARY KEY AUTOINCREMENT,
    channel     TEXT NOT NULL DEFAULT 'stable',  -- stable / beta / nightly
    version     TEXT NOT NULL,                   -- e.g. 2026.11.13-1 or a semver
    machine     TEXT,                            -- se / plus / lc3 / universal
    title       TEXT,                            -- short human name of the update
    notes       TEXT,                            -- markdown-ish release notes
    created_at  REAL NOT NULL,
    published_by TEXT,                           -- who/what pushed it
    yanked      INTEGER NOT NULL DEFAULT 0,       -- hidden from clients if 1
    UNIQUE(channel, version)
);

-- One artifact = one file that belongs to a release. kind tells the client how
-- to apply it. 'repo' artifacts are app/package repositories; 'app' is a single
-- program; 'kernel' and 'apps_store' are the raw boot payloads; 'image' is a
-- whole disk image (optional, large).
CREATE TABLE IF NOT EXISTS artifacts (
    id          INTEGER PRIMARY KEY AUTOINCREMENT,
    release_id  INTEGER NOT NULL,
    kind        TEXT NOT NULL,                   -- kernel|apps_store|app|repo|loader|image|file
    name        TEXT NOT NULL,                   -- logical name, e.g. 'SHELL' or 'kernel.pex'
    path        TEXT,                            -- where it belongs on the PlusOS volume
    sha256      TEXT NOT NULL,                   -- content hash (also the on-disk filename)
    size        INTEGER NOT NULL,
    info        TEXT,                            -- free JSON: per-artifact notes / changelog
    created_at  REAL NOT NULL,
    FOREIGN KEY(release_id) REFERENCES releases(id)
);
CREATE INDEX IF NOT EXISTS idx_art_release ON artifacts(release_id);

-- Admin control-panel sessions (cookie tokens).
CREATE TABLE IF NOT EXISTS admin_sessions (
    token       TEXT PRIMARY KEY,
    created_at  REAL NOT NULL,
    last_seen   REAL NOT NULL,
    ip          TEXT
);

-- Audit trail: every publish, yank, prune, restart, shell command.
CREATE TABLE IF NOT EXISTS audit (
    id          INTEGER PRIMARY KEY AUTOINCREMENT,
    ts          REAL NOT NULL,
    actor       TEXT,          -- 'publisher', 'admin', 'system'
    action      TEXT NOT NULL,
    detail      TEXT
);

-- Key/value settings (admin password hash, publish key hash, panel options).
CREATE TABLE IF NOT EXISTS settings (
    key         TEXT PRIMARY KEY,
    value       TEXT
);

-- AI-written artefacts about a release: 'message' (catalog blurb) or
-- 'app_doc' (per-application Markdown changelog). Generated at publish time.
CREATE TABLE IF NOT EXISTS ai_docs (
    id          INTEGER PRIMARY KEY AUTOINCREMENT,
    release_id  INTEGER NOT NULL,
    kind        TEXT NOT NULL,          -- message | app_doc
    name        TEXT,                   -- app name for app_doc, '' for message
    body        TEXT NOT NULL,          -- markdown / text
    created_at  REAL NOT NULL,
    FOREIGN KEY(release_id) REFERENCES releases(id)
);
CREATE INDEX IF NOT EXISTS idx_aidoc_release ON ai_docs(release_id);

-- Chat transcript with the Update Assistant. session is any opaque id (a native
-- client id, a browser session, etc.); role is user|assistant|system.
CREATE TABLE IF NOT EXISTS chat (
    id          INTEGER PRIMARY KEY AUTOINCREMENT,
    session     TEXT NOT NULL,
    role        TEXT NOT NULL,
    content     TEXT NOT NULL,
    created_at  REAL NOT NULL
);
CREATE INDEX IF NOT EXISTS idx_chat_session ON chat(session, id);
"""


def get() -> sqlite3.Connection:
    global _conn
    if _conn is None:
        _conn = sqlite3.connect(DB_PATH, check_same_thread=False)
        _conn.row_factory = sqlite3.Row
        _conn.execute("PRAGMA journal_mode=WAL;")
        _conn.execute("PRAGMA foreign_keys=ON;")
        _conn.executescript(SCHEMA)
        _conn.commit()
    return _conn


# ------------------------------------------------------------------ settings

def set_setting(key: str, value: str) -> None:
    with _lock:
        c = get()
        c.execute(
            "INSERT INTO settings(key,value) VALUES(?,?) "
            "ON CONFLICT(key) DO UPDATE SET value=excluded.value",
            (key, value),
        )
        c.commit()


def get_setting(key: str, default: Optional[str] = None) -> Optional[str]:
    row = get().execute("SELECT value FROM settings WHERE key=?", (key,)).fetchone()
    return row["value"] if row else default


# ------------------------------------------------------------------ releases

def create_release(channel, version, machine, title, notes, published_by) -> int:
    with _lock:
        c = get()
        cur = c.execute(
            """INSERT INTO releases(channel,version,machine,title,notes,created_at,published_by)
               VALUES(?,?,?,?,?,?,?)
               ON CONFLICT(channel,version) DO UPDATE SET
                   machine=excluded.machine, title=excluded.title,
                   notes=excluded.notes, created_at=excluded.created_at,
                   published_by=excluded.published_by, yanked=0""",
            (channel, version, machine, title, notes, time.time(), published_by),
        )
        c.commit()
        if cur.lastrowid:
            rid = cur.lastrowid
            # ON CONFLICT path doesn't give lastrowid reliably; look it up.
            row = c.execute(
                "SELECT id FROM releases WHERE channel=? AND version=?",
                (channel, version),
            ).fetchone()
            return row["id"]
        row = c.execute(
            "SELECT id FROM releases WHERE channel=? AND version=?",
            (channel, version),
        ).fetchone()
        return row["id"]


def clear_release_artifacts(release_id: int) -> None:
    """Drop the artifact rows for a release so a re-publish replaces them."""
    with _lock:
        c = get()
        c.execute("DELETE FROM artifacts WHERE release_id=?", (release_id,))
        c.commit()


def get_release(rid: int) -> Optional[sqlite3.Row]:
    return get().execute("SELECT * FROM releases WHERE id=?", (rid,)).fetchone()


def release_by_version(channel: str, version: str) -> Optional[sqlite3.Row]:
    return get().execute(
        "SELECT * FROM releases WHERE channel=? AND version=?", (channel, version)
    ).fetchone()


def list_releases(channel: Optional[str] = None, include_yanked: bool = False):
    q = "SELECT * FROM releases"
    cond, args = [], []
    if channel:
        cond.append("channel=?")
        args.append(channel)
    if not include_yanked:
        cond.append("yanked=0")
    if cond:
        q += " WHERE " + " AND ".join(cond)
    q += " ORDER BY created_at DESC"
    return get().execute(q, args).fetchall()


def latest_release(channel: str, machine: Optional[str] = None) -> Optional[sqlite3.Row]:
    q = "SELECT * FROM releases WHERE channel=? AND yanked=0"
    args = [channel]
    if machine:
        q += " AND (machine IS NULL OR machine=? OR machine='universal')"
        args.append(machine)
    q += " ORDER BY created_at DESC LIMIT 1"
    return get().execute(q, args).fetchone()


def set_yanked(rid: int, yanked: bool) -> None:
    with _lock:
        c = get()
        c.execute("UPDATE releases SET yanked=? WHERE id=?", (1 if yanked else 0, rid))
        c.commit()


def delete_release(rid: int) -> None:
    with _lock:
        c = get()
        c.execute("DELETE FROM artifacts WHERE release_id=?", (rid,))
        c.execute("DELETE FROM releases WHERE id=?", (rid,))
        c.commit()


# ------------------------------------------------------------------ artifacts

def add_artifact(release_id, kind, name, path, sha256, size, info) -> int:
    with _lock:
        c = get()
        cur = c.execute(
            """INSERT INTO artifacts(release_id,kind,name,path,sha256,size,info,created_at)
               VALUES(?,?,?,?,?,?,?,?)""",
            (release_id, kind, name, path, sha256, size,
             json.dumps(info) if info is not None else None, time.time()),
        )
        c.commit()
        return cur.lastrowid


def release_artifacts(release_id: int):
    return get().execute(
        "SELECT * FROM artifacts WHERE release_id=? ORDER BY kind, name",
        (release_id,),
    ).fetchall()


def artifact_by_hash(sha256: str) -> Optional[sqlite3.Row]:
    return get().execute(
        "SELECT * FROM artifacts WHERE sha256=? LIMIT 1", (sha256,)
    ).fetchone()


def orphan_hashes() -> list[str]:
    """Content hashes on disk that no artifact row references any more."""
    referenced = {r["sha256"] for r in get().execute("SELECT DISTINCT sha256 FROM artifacts").fetchall()}
    on_disk = set(os.listdir(ARTIFACT_DIR)) if os.path.isdir(ARTIFACT_DIR) else set()
    return sorted(on_disk - referenced)


# ------------------------------------------------------------------ AI docs

def set_ai_doc(release_id: int, kind: str, name: str, body: str) -> int:
    """Store (replacing any existing same kind+name) an AI doc for a release."""
    with _lock:
        c = get()
        c.execute("DELETE FROM ai_docs WHERE release_id=? AND kind=? AND name=?",
                  (release_id, kind, name or ""))
        cur = c.execute(
            "INSERT INTO ai_docs(release_id,kind,name,body,created_at) VALUES(?,?,?,?,?)",
            (release_id, kind, name or "", body, time.time()))
        c.commit()
        return cur.lastrowid


def release_ai_message(release_id: int) -> Optional[str]:
    row = get().execute(
        "SELECT body FROM ai_docs WHERE release_id=? AND kind='message' LIMIT 1",
        (release_id,)).fetchone()
    return row["body"] if row else None


def release_app_docs(release_id: int):
    return get().execute(
        "SELECT name, body FROM ai_docs WHERE release_id=? AND kind='app_doc' ORDER BY name",
        (release_id,)).fetchall()


# ------------------------------------------------------------------ chat

def add_chat(session: str, role: str, content: str) -> None:
    with _lock:
        c = get()
        c.execute("INSERT INTO chat(session,role,content,created_at) VALUES(?,?,?,?)",
                  (session, role, content, time.time()))
        c.commit()


def chat_history(session: str, limit: int = 40):
    rows = get().execute(
        "SELECT role, content FROM chat WHERE session=? ORDER BY id DESC LIMIT ?",
        (session, limit)).fetchall()
    return list(reversed(rows))


# ------------------------------------------------------------------ admin sessions

def create_admin_session(ip: Optional[str]) -> str:
    token = secrets.token_hex(24)
    now = time.time()
    with _lock:
        c = get()
        c.execute(
            "INSERT INTO admin_sessions(token,created_at,last_seen,ip) VALUES(?,?,?,?)",
            (token, now, now, ip),
        )
        c.commit()
    return token


def touch_admin_session(token: str) -> Optional[sqlite3.Row]:
    row = get().execute("SELECT * FROM admin_sessions WHERE token=?", (token,)).fetchone()
    if not row:
        return None
    # Expire after 12h of inactivity.
    if time.time() - row["last_seen"] > 12 * 3600:
        drop_admin_session(token)
        return None
    with _lock:
        get().execute("UPDATE admin_sessions SET last_seen=? WHERE token=?", (time.time(), token))
        get().commit()
    return row


def drop_admin_session(token: str) -> None:
    with _lock:
        get().execute("DELETE FROM admin_sessions WHERE token=?", (token,))
        get().commit()


# ------------------------------------------------------------------ audit

def audit(actor: str, action: str, detail: str = "") -> None:
    with _lock:
        c = get()
        c.execute(
            "INSERT INTO audit(ts,actor,action,detail) VALUES(?,?,?,?)",
            (time.time(), actor, action, detail),
        )
        c.commit()


def recent_audit(limit: int = 100):
    return get().execute(
        "SELECT * FROM audit ORDER BY id DESC LIMIT ?", (limit,)
    ).fetchall()


# ------------------------------------------------------------------ stats

def stats() -> dict:
    c = get()
    rel = c.execute("SELECT COUNT(*) n FROM releases WHERE yanked=0").fetchone()["n"]
    art = c.execute("SELECT COUNT(*) n FROM artifacts").fetchone()["n"]
    bytes_ = 0
    if os.path.isdir(ARTIFACT_DIR):
        for f in os.listdir(ARTIFACT_DIR):
            try:
                bytes_ += os.path.getsize(os.path.join(ARTIFACT_DIR, f))
            except OSError:
                pass
    channels = [r["channel"] for r in c.execute(
        "SELECT DISTINCT channel FROM releases ORDER BY channel").fetchall()]
    return {"releases": rel, "artifacts": art, "artifact_bytes": bytes_, "channels": channels}

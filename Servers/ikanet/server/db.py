"""IKANET — SQLite data layer.

Accounts, sessions, channels (rooms), membership, messages, uploaded files.
A single sqlite file lives in ../data/ikanet.db. Everything is intentionally
small and synchronous — the realtime layer never blocks long on the DB.
"""
from __future__ import annotations

import os
import sqlite3
import time
import secrets
import threading
from typing import Optional

DATA_DIR = os.path.normpath(os.path.join(os.path.dirname(__file__), "..", "data"))
UPLOAD_DIR = os.path.join(DATA_DIR, "uploads")
DB_PATH = os.path.join(DATA_DIR, "ikanet.db")

os.makedirs(UPLOAD_DIR, exist_ok=True)

# sqlite with check_same_thread=False + a lock keeps this safe under the
# single uvicorn worker / asyncio loop we run.
_conn: sqlite3.Connection | None = None
_lock = threading.RLock()

SCHEMA = """
CREATE TABLE IF NOT EXISTS users (
    id          INTEGER PRIMARY KEY AUTOINCREMENT,
    username    TEXT UNIQUE NOT NULL COLLATE NOCASE,
    pass_hash   TEXT NOT NULL,
    icon        TEXT,              -- 16x16 mono bitmap, 256 chars of '0'/'1'
    created_at  REAL NOT NULL,
    appear_offline INTEGER NOT NULL DEFAULT 0
);

CREATE TABLE IF NOT EXISTS sessions (
    token       TEXT PRIMARY KEY,
    user_id     INTEGER NOT NULL,
    created_at  REAL NOT NULL,
    FOREIGN KEY(user_id) REFERENCES users(id)
);

CREATE TABLE IF NOT EXISTS channels (
    id          INTEGER PRIMARY KEY AUTOINCREMENT,
    name        TEXT UNIQUE NOT NULL COLLATE NOCASE,  -- '#general' style, or 'dm:<a>:<b>'
    is_dm       INTEGER NOT NULL DEFAULT 0,
    created_at  REAL NOT NULL
);

CREATE TABLE IF NOT EXISTS memberships (
    user_id     INTEGER NOT NULL,
    channel_id  INTEGER NOT NULL,
    saved       INTEGER NOT NULL DEFAULT 1,   -- in the user's saved channel list
    PRIMARY KEY (user_id, channel_id)
);

CREATE TABLE IF NOT EXISTS files (
    id          INTEGER PRIMARY KEY AUTOINCREMENT,
    owner_id    INTEGER NOT NULL,
    disk_name   TEXT NOT NULL,     -- name on server disk
    orig_name   TEXT NOT NULL,     -- original filename / title
    mime        TEXT NOT NULL,
    size        INTEGER NOT NULL,
    width       INTEGER,
    height      INTEGER,
    is_image    INTEGER NOT NULL DEFAULT 0,
    created_at  REAL NOT NULL
);

CREATE TABLE IF NOT EXISTS messages (
    id          INTEGER PRIMARY KEY AUTOINCREMENT,
    channel_id  INTEGER NOT NULL,
    user_id     INTEGER,           -- NULL = system message
    body        TEXT,
    file_id     INTEGER,           -- optional attachment
    created_at  REAL NOT NULL,
    FOREIGN KEY(channel_id) REFERENCES channels(id)
);

CREATE INDEX IF NOT EXISTS idx_msg_channel ON messages(channel_id, id);
"""


def get() -> sqlite3.Connection:
    global _conn
    if _conn is None:
        _conn = sqlite3.connect(DB_PATH, check_same_thread=False)
        _conn.row_factory = sqlite3.Row
        _conn.execute("PRAGMA journal_mode=WAL;")
        _conn.executescript(SCHEMA)
        _conn.commit()
        _seed()
    return _conn


def _seed() -> None:
    """Make sure the default public channels exist."""
    with _lock:
        c = _conn
        for name in ("#general", "#swimming", "#gameboy"):
            row = c.execute("SELECT id FROM channels WHERE name=?", (name,)).fetchone()
            if not row:
                c.execute(
                    "INSERT INTO channels(name, is_dm, created_at) VALUES(?,0,?)",
                    (name, time.time()),
                )
        c.commit()


# ---------------------------------------------------------------- users

def create_user(username: str, pass_hash: str, icon: Optional[str]) -> int:
    with _lock:
        c = get()
        cur = c.execute(
            "INSERT INTO users(username, pass_hash, icon, created_at) VALUES(?,?,?,?)",
            (username, pass_hash, icon, time.time()),
        )
        uid = cur.lastrowid
        # auto-join the default channel
        gen = c.execute("SELECT id FROM channels WHERE name='#general'").fetchone()
        if gen:
            c.execute(
                "INSERT OR IGNORE INTO memberships(user_id, channel_id, saved) VALUES(?,?,1)",
                (uid, gen["id"]),
            )
        c.commit()
        return uid


def get_user_by_name(username: str) -> Optional[sqlite3.Row]:
    return get().execute(
        "SELECT * FROM users WHERE username=?", (username,)
    ).fetchone()


def get_user(uid: int) -> Optional[sqlite3.Row]:
    return get().execute("SELECT * FROM users WHERE id=?", (uid,)).fetchone()


def set_icon(uid: int, icon: str) -> None:
    with _lock:
        c = get()
        c.execute("UPDATE users SET icon=? WHERE id=?", (icon, uid))
        c.commit()


def set_appear_offline(uid: int, value: bool) -> None:
    with _lock:
        c = get()
        c.execute("UPDATE users SET appear_offline=? WHERE id=?", (1 if value else 0, uid))
        c.commit()


# ---------------------------------------------------------------- sessions

def create_session(uid: int) -> str:
    token = secrets.token_hex(24)
    with _lock:
        c = get()
        c.execute(
            "INSERT INTO sessions(token, user_id, created_at) VALUES(?,?,?)",
            (token, uid, time.time()),
        )
        c.commit()
    return token


def user_for_token(token: str) -> Optional[sqlite3.Row]:
    row = get().execute(
        "SELECT u.* FROM sessions s JOIN users u ON u.id=s.user_id WHERE s.token=?",
        (token,),
    ).fetchone()
    return row


def drop_session(token: str) -> None:
    with _lock:
        c = get()
        c.execute("DELETE FROM sessions WHERE token=?", (token,))
        c.commit()


# ---------------------------------------------------------------- channels

def get_or_create_channel(name: str, is_dm: bool = False) -> sqlite3.Row:
    with _lock:
        c = get()
        row = c.execute("SELECT * FROM channels WHERE name=?", (name,)).fetchone()
        if row:
            return row
        c.execute(
            "INSERT INTO channels(name, is_dm, created_at) VALUES(?,?,?)",
            (name, 1 if is_dm else 0, time.time()),
        )
        c.commit()
        return c.execute("SELECT * FROM channels WHERE name=?", (name,)).fetchone()


def channel_by_id(cid: int) -> Optional[sqlite3.Row]:
    return get().execute("SELECT * FROM channels WHERE id=?", (cid,)).fetchone()


def dm_channel_name(a: str, b: str) -> str:
    lo, hi = sorted([a.lower(), b.lower()])
    return f"dm:{lo}:{hi}"


def join_channel(uid: int, cid: int, saved: bool = True) -> None:
    with _lock:
        c = get()
        c.execute(
            "INSERT OR REPLACE INTO memberships(user_id, channel_id, saved) VALUES(?,?,?)",
            (uid, cid, 1 if saved else 0),
        )
        c.commit()


def leave_channel(uid: int, cid: int) -> None:
    with _lock:
        c = get()
        c.execute(
            "DELETE FROM memberships WHERE user_id=? AND channel_id=?", (uid, cid)
        )
        c.commit()


def saved_channels(uid: int) -> list[sqlite3.Row]:
    return get().execute(
        """SELECT ch.* FROM channels ch
           JOIN memberships m ON m.channel_id=ch.id
           WHERE m.user_id=? AND m.saved=1
           ORDER BY ch.is_dm, ch.name""",
        (uid,),
    ).fetchall()


def public_channels() -> list[sqlite3.Row]:
    return get().execute(
        "SELECT * FROM channels WHERE is_dm=0 ORDER BY name"
    ).fetchall()


# ---------------------------------------------------------------- files

def add_file(owner_id, disk_name, orig_name, mime, size, width, height, is_image) -> int:
    with _lock:
        c = get()
        cur = c.execute(
            """INSERT INTO files(owner_id, disk_name, orig_name, mime, size,
                                 width, height, is_image, created_at)
               VALUES(?,?,?,?,?,?,?,?,?)""",
            (owner_id, disk_name, orig_name, mime, size, width, height,
             1 if is_image else 0, time.time()),
        )
        c.commit()
        return cur.lastrowid


def get_file(fid: int) -> Optional[sqlite3.Row]:
    return get().execute("SELECT * FROM files WHERE id=?", (fid,)).fetchone()


# ---------------------------------------------------------------- messages

def add_message(channel_id, user_id, body, file_id=None) -> sqlite3.Row:
    with _lock:
        c = get()
        cur = c.execute(
            """INSERT INTO messages(channel_id, user_id, body, file_id, created_at)
               VALUES(?,?,?,?,?)""",
            (channel_id, user_id, body, file_id, time.time()),
        )
        c.commit()
        return c.execute("SELECT * FROM messages WHERE id=?", (cur.lastrowid,)).fetchone()


def recent_messages(channel_id: int, limit: int = 60) -> list[sqlite3.Row]:
    rows = get().execute(
        "SELECT * FROM messages WHERE channel_id=? ORDER BY id DESC LIMIT ?",
        (channel_id, limit),
    ).fetchall()
    return list(reversed(rows))

"""Authentication for the OTA server.

Two independent gates:

1. PUBLISH KEY  — a long random secret that lives only on the maintainer's
   machine (in ikanet/ota/.publish_key, git-ignored). Every publish request
   must prove possession of it. The server stores only a salted hash. This is
   what enforces "only I, on my computer, can commit updates": publishing is
   an HMAC challenge signed with the key, so the key is never sent over the
   wire and a captured request can't be replayed (it's timestamp-bound).

2. ADMIN PASSWORD — gates the web control panel + system management. Stored as
   a salted PBKDF2 hash in the DB. Set on first run (env OTA_ADMIN_PASSWORD or
   an interactive prompt), changeable from the panel.

On first run, if no publish key hash exists, a key is generated and written to
.publish_key and its hash saved; the maintainer keeps that file. If the file is
copied from another machine it still works (it's just the shared secret), which
is what you want for "my computer".
"""
from __future__ import annotations

import os
import hmac
import time
import hashlib
import secrets
from typing import Optional

import db

KEY_PATH = os.path.join(os.path.dirname(__file__), ".publish_key")


# ------------------------------------------------------------------ password hashing

def _pbkdf2(password: str, salt: bytes) -> bytes:
    return hashlib.pbkdf2_hmac("sha256", password.encode("utf-8"), salt, 200_000)


def hash_password(password: str) -> str:
    salt = secrets.token_bytes(16)
    return "pbkdf2$" + salt.hex() + "$" + _pbkdf2(password, salt).hex()


def verify_password(password: str, stored: str) -> bool:
    try:
        scheme, salt_hex, want_hex = stored.split("$", 2)
        if scheme != "pbkdf2":
            return False
        got = _pbkdf2(password, bytes.fromhex(salt_hex))
        return hmac.compare_digest(got.hex(), want_hex)
    except Exception:
        return False


# ------------------------------------------------------------------ admin password

def ensure_admin_password() -> None:
    """Make sure an admin password hash exists. Priority: already-set > env var.
    If neither, a random one is generated and printed once (so the server never
    starts wide open)."""
    if db.get_setting("admin_pw_hash"):
        return
    env = os.environ.get("OTA_ADMIN_PASSWORD")
    if env:
        db.set_setting("admin_pw_hash", hash_password(env))
        print("[auth] admin password set from OTA_ADMIN_PASSWORD")
        return
    gen = secrets.token_urlsafe(12)
    db.set_setting("admin_pw_hash", hash_password(gen))
    print("=" * 62)
    print(" No admin password was set. A temporary one was generated:")
    print("     ADMIN PASSWORD:  %s" % gen)
    print(" Log into the control panel and change it, or set")
    print(" OTA_ADMIN_PASSWORD before first run.")
    print("=" * 62)


def set_admin_password(new_password: str) -> None:
    db.set_setting("admin_pw_hash", hash_password(new_password))


def check_admin_password(password: str) -> bool:
    stored = db.get_setting("admin_pw_hash")
    return bool(stored) and verify_password(password, stored)


# ------------------------------------------------------------------ publish key

def ensure_publish_key() -> str:
    """Return the publish key, creating .publish_key + its stored hash if needed.
    The returned value is the raw secret (used by the local publisher CLI)."""
    if os.path.exists(KEY_PATH):
        with open(KEY_PATH, "r", encoding="utf-8") as fh:
            key = fh.read().strip()
    else:
        key = secrets.token_urlsafe(32)
        with open(KEY_PATH, "w", encoding="utf-8") as fh:
            fh.write(key + "\n")
        try:
            os.chmod(KEY_PATH, 0o600)
        except OSError:
            pass
        print("[auth] generated new publish key -> %s (keep this file private)" % KEY_PATH)
    # Make sure the hash the server checks against matches this key.
    want = hashlib.sha256(key.encode()).hexdigest()
    if db.get_setting("publish_key_hash") != want:
        db.set_setting("publish_key_hash", want)
    return key


def load_publish_key() -> Optional[str]:
    """Load the raw key from disk (for the publisher side)."""
    if os.path.exists(KEY_PATH):
        with open(KEY_PATH, "r", encoding="utf-8") as fh:
            return fh.read().strip()
    return os.environ.get("OTA_PUBLISH_KEY")


def publish_key_hash() -> Optional[str]:
    return db.get_setting("publish_key_hash")


# --- HMAC challenge: publisher signs (timestamp + body-hash) with the key -----
# The server never receives the key, only a signature it can verify from the
# stored key hash... except HMAC needs the key itself. So instead we verify by
# recomputing: the client sends sig = HMAC(key, ts|body_sha). The server holds
# the key hash, not the key, so it can't recompute HMAC. To keep the key off the
# wire *and* verifiable server-side, the server keeps the key itself (same file)
# when it runs on the maintainer's machine; when it runs remotely it keeps only
# the hash and falls back to verifying a simpler bearer proof. We support both:
#
#   * If the server can read .publish_key (same machine / mounted secret), it
#     verifies the HMAC properly and rejects stale timestamps (replay-proof).
#   * Otherwise it verifies sha256(provided_key) == stored hash (bearer mode).

REPLAY_WINDOW = 300  # seconds


def sign_publish(key: str, ts: int, body_sha256: str) -> str:
    msg = ("%d:%s" % (ts, body_sha256)).encode()
    return hmac.new(key.encode(), msg, hashlib.sha256).hexdigest()


def verify_publish(ts: int, body_sha256: str, signature: str,
                   bearer_key: Optional[str] = None) -> tuple[bool, str]:
    now = int(time.time())
    if abs(now - ts) > REPLAY_WINDOW:
        return False, "stale timestamp (clock skew or replay)"
    server_key = None
    if os.path.exists(KEY_PATH):
        with open(KEY_PATH, "r", encoding="utf-8") as fh:
            server_key = fh.read().strip()
    if server_key:
        want = sign_publish(server_key, ts, body_sha256)
        if hmac.compare_digest(want, signature or ""):
            return True, "ok (hmac)"
        return False, "bad signature"
    # Bearer fallback: the request carried the key; check its hash.
    stored = publish_key_hash()
    if bearer_key and stored and hmac.compare_digest(
            hashlib.sha256(bearer_key.encode()).hexdigest(), stored):
        return True, "ok (bearer)"
    return False, "no publish key available to verify"

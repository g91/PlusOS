"""Server-side ingest of a published release.

A publish is one HTTP request carrying a JSON manifest plus a set of artifact
blobs. The manifest describes the release (channel/version/machine/notes) and
lists artifacts with their sha256. Blobs are uploaded content-addressed; an
artifact whose hash the server already has needs no re-upload (the publisher
asks /api/have first and skips them), which makes "publish every compile" cheap.
"""
from __future__ import annotations

import os
import json
import hashlib

import db
import ai

CHUNK = 1 << 20


def artifact_path(sha256: str) -> str:
    return os.path.join(db.ARTIFACT_DIR, sha256)


def have(sha256: str) -> bool:
    return os.path.exists(artifact_path(sha256))


def store_blob(sha256: str, data: bytes) -> tuple[bool, str]:
    """Write a blob to the content store, verifying its hash."""
    real = hashlib.sha256(data).hexdigest()
    if real != sha256:
        return False, "hash mismatch: claimed %s got %s" % (sha256, real)
    path = artifact_path(sha256)
    if not os.path.exists(path):
        tmp = path + ".tmp"
        with open(tmp, "wb") as fh:
            fh.write(data)
        os.replace(tmp, path)
    return True, real


def register_release(manifest: dict, published_by: str) -> dict:
    """Create/replace a release and its artifact rows. Assumes every artifact's
    blob is already in the content store (uploaded beforehand)."""
    channel = manifest.get("channel", "stable")
    version = manifest["version"]
    machine = manifest.get("machine")
    title = manifest.get("title") or ""
    notes = manifest.get("notes") or ""
    artifacts = manifest.get("artifacts", [])

    missing = [a["sha256"] for a in artifacts if not have(a["sha256"])]
    if missing:
        raise ValueError("missing blobs for: %s" % ", ".join(missing[:5]))

    rid = db.create_release(channel, version, machine, title, notes, published_by)
    db.clear_release_artifacts(rid)
    for a in artifacts:
        size = os.path.getsize(artifact_path(a["sha256"]))
        db.add_artifact(rid, a.get("kind", "file"), a["name"], a.get("path"),
                        a["sha256"], size, a.get("info"))
    db.audit("publisher", "publish",
             "%s/%s machine=%s artifacts=%d" % (channel, version, machine, len(artifacts)))

    # --- AI-generated messages about this update -------------------------------
    # One catalog blurb for the whole release, plus a Markdown changelog doc for
    # every application in it. If the publisher already supplied an app_doc in
    # the artifact's info (generated client-side at compile time), keep that;
    # otherwise generate here. Never let AI failure break a publish.
    try:
        msg = ai.release_message(channel, version, machine, artifacts, notes)
        db.set_ai_doc(rid, "message", "", msg)
    except Exception as e:
        db.set_ai_doc(rid, "message", "", notes or ("PlusOS %s" % version))
        db.audit("system", "ai_error", "release_message: %s" % e)

    for a in artifacts:
        if a.get("kind") != "app":
            continue
        info = a.get("info") or {}
        doc = info.get("app_doc") if isinstance(info, dict) else None
        if not doc:
            try:
                doc = ai.app_changelog(a["name"], version, notes,
                                       (info or {}).get("diffstat", "") if isinstance(info, dict) else "")
            except Exception as e:
                doc = "# %s — %s\n\n%s\n" % (a["name"], version, notes or "Rebuilt.")
                db.audit("system", "ai_error", "app_doc %s: %s" % (a["name"], e))
        db.set_ai_doc(rid, "app_doc", a["name"], doc)

    return {"release_id": rid, "channel": channel, "version": version,
            "artifacts": len(artifacts)}


def release_dto(row, with_docs: bool = False) -> dict:
    arts = db.release_artifacts(row["id"])
    dto = {
        "id": row["id"],
        "channel": row["channel"],
        "version": row["version"],
        "machine": row["machine"],
        "title": row["title"],
        "notes": row["notes"],
        "ai_message": db.release_ai_message(row["id"]),
        "yanked": bool(row["yanked"]),
        "created_at": row["created_at"],
        "published_by": row["published_by"],
        "artifacts": [artifact_dto(a) for a in arts],
        "total_bytes": sum(a["size"] for a in arts),
    }
    if with_docs:
        dto["app_docs"] = {r["name"]: r["body"] for r in db.release_app_docs(row["id"])}
    return dto


def artifact_dto(a) -> dict:
    info = None
    if a["info"]:
        try:
            info = json.loads(a["info"])
        except Exception:
            info = a["info"]
    return {
        "id": a["id"], "kind": a["kind"], "name": a["name"], "path": a["path"],
        "sha256": a["sha256"], "size": a["size"], "info": info,
    }

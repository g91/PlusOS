#!/usr/bin/env python3
"""PlusOS OTA publisher — run on every compile to push a new release.

It collects the build outputs (kernel, boot loader, raw app store, individual
apps, and any extra repos), content-hashes them, uploads only the blobs the
server does not already have, and registers a signed release. Signing uses the
local publish key (otaserver/.publish_key or $OTA_PUBLIC_KEY), so only your
machine can publish.

Typical use from the Makefile (see Makefile.p68 `ota` target):

    python otaserver/publish_cli.py \
        --server https://ota.d0a.net \
        --channel stable --machine se \
        --build build_p68 \
        --title "Nightly" --notes-file RELEASE_NOTES.md

If --version is omitted it is derived as <UTC-date>-<n> (auto-incrementing the
suffix if that date already has a release on the channel), or from `git describe`
with --version-from-git.

This script is dependency-light: only the `requests`-free stdlib + urllib, so it
runs anywhere Python does without installing the server's deps.
"""
from __future__ import annotations

import os
import sys
import json
import time
import glob
import hashlib
import argparse
import datetime
import subprocess
import urllib.request
import urllib.error

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.normpath(os.path.join(HERE, ".."))

# Optional AI endpoint for generating per-app docs at compile time (same vars as
# the server's ai.py HTTP backend). Without it, a template doc is written.
AI_BASE_URL = os.environ.get("OTA_AI_BASE_URL")
AI_API_KEY = os.environ.get("OTA_AI_API_KEY", "none")
AI_MODEL = os.environ.get("OTA_AI_MODEL", "lfm2-2.6b")


# --- signing (mirrors auth.sign_publish; kept standalone so the CLI has no deps) ---
def load_key() -> str:
    kp = os.path.join(HERE, ".publish_key")
    if os.path.exists(kp):
        with open(kp, "r", encoding="utf-8") as fh:
            return fh.read().strip()
    env = os.environ.get("OTA_PUBLISH_KEY")
    if env:
        return env.strip()
    sys.exit("publish_cli: no publish key found (otaserver/.publish_key or $OTA_PUBLISH_KEY). "
             "Run the server once to generate one, or copy it from your main machine.")


def sign(key: str, ts: int, body_sha: str) -> str:
    import hmac
    return hmac.new(key.encode(), ("%d:%s" % (ts, body_sha)).encode(), hashlib.sha256).hexdigest()


def sha256_file(path: str) -> str:
    h = hashlib.sha256()
    with open(path, "rb") as fh:
        for chunk in iter(lambda: fh.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


# --- HTTP helpers --------------------------------------------------------------
def _req(method, url, data=None, headers=None, timeout=120):
    req = urllib.request.Request(url, data=data, method=method, headers=headers or {})
    try:
        with urllib.request.urlopen(req, timeout=timeout) as r:
            return r.status, r.read()
    except urllib.error.HTTPError as e:
        return e.code, e.read()
    except urllib.error.URLError as e:
        sys.exit("publish_cli: cannot reach %s (%s)" % (url, e.reason))


def server_has(server, sha) -> bool:
    status, body = _req("GET", server + "/api/have?sha256=" + sha)
    return status == 200 and json.loads(body).get("have")


def upload_blob(server, key, path, sha):
    with open(path, "rb") as fh:
        data = fh.read()
    ts = int(time.time())
    body_sha = hashlib.sha256(data).hexdigest()
    headers = {
        "Content-Type": "application/octet-stream",
        "X-OTA-Ts": str(ts),
        "X-OTA-Sha256": sha,
        "X-OTA-Sig": sign(key, ts, body_sha),
        "X-OTA-Key": key,  # bearer fallback for remote servers without the key file
    }
    status, resp = _req("POST", server + "/api/blob", data=data, headers=headers)
    if status != 200:
        sys.exit("publish_cli: blob upload failed (%d): %s" % (status, resp.decode("utf-8", "replace")))


def publish_manifest(server, key, manifest: dict):
    body = json.dumps(manifest).encode("utf-8")
    ts = int(time.time())
    body_sha = hashlib.sha256(body).hexdigest()
    headers = {
        "Content-Type": "application/json",
        "X-OTA-Ts": str(ts),
        "X-OTA-Sig": sign(key, ts, body_sha),
        "X-OTA-Key": key,
    }
    status, resp = _req("POST", server + "/api/publish", data=body, headers=headers)
    if status != 200:
        sys.exit("publish_cli: publish failed (%d): %s" % (status, resp.decode("utf-8", "replace")))
    return json.loads(resp)


# --- artifact discovery --------------------------------------------------------
def discover(build: str, extra_repos: list[str]) -> list[dict]:
    """Find the standard PlusOS build outputs under <build>/."""
    arts = []

    def add(path, kind, name, dest):
        if path and os.path.isfile(path):
            arts.append({"kind": kind, "name": name, "path_src": path, "dest": dest})

    add(os.path.join(build, "kernel", "kernel.pex"), "kernel", "kernel.pex", "/.boot/kernel.pex")
    add(os.path.join(build, "boot", "loader.bin"), "loader", "loader.bin", "/.boot/loader.bin")

    # Individual apps: build_p68/apps/*.pex (or whatever mk_apps emits). We scan
    # for .pex files outside the kernel dir and treat each as an app artifact.
    for pex in glob.glob(os.path.join(build, "apps", "*")) + glob.glob(os.path.join(build, "app", "*")):
        if os.path.isfile(pex) and not pex.endswith(".map"):
            name = os.path.splitext(os.path.basename(pex))[0].upper()
            add(pex, "app", name, "/bin/" + name)

    # A staged raw app store, if present.
    for cand in ("apps.raw", "appstore.bin", "raw_apps.bin"):
        p = os.path.join(build, cand)
        add(p, "apps_store", cand, None)

    # Extra repositories: each --repo DIR becomes one 'repo' artifact (a tarball).
    for repo in extra_repos:
        if os.path.isdir(repo):
            import tarfile, tempfile
            tmp = tempfile.NamedTemporaryFile(delete=False, suffix=".tar.gz")
            tmp.close()
            with tarfile.open(tmp.name, "w:gz") as tf:
                tf.add(repo, arcname=os.path.basename(repo.rstrip("/\\")))
            add(tmp.name, "repo", os.path.basename(repo.rstrip("/\\")) + ".tar.gz",
                "/repos/" + os.path.basename(repo.rstrip("/\\")))
        elif os.path.isfile(repo):
            add(repo, "repo", os.path.basename(repo), "/repos/" + os.path.basename(repo))
    return arts


def git_diffstat(paths=None) -> str:
    """Short diffstat since the last commit, to feed the changelog AI. Best
    effort; returns '' if git isn't available."""
    try:
        cmd = ["git", "diff", "--stat", "HEAD~1", "HEAD"]
        if paths:
            cmd += ["--"] + paths
        return subprocess.check_output(cmd, cwd=REPO, text=True,
                                       stderr=subprocess.DEVNULL).strip()
    except Exception:
        return ""


def ai_app_doc(name, version, notes, diffstat) -> str:
    """Generate a Markdown changelog for one app. Uses an OpenAI-compatible
    endpoint if OTA_AI_BASE_URL is set, else a deterministic template."""
    if AI_BASE_URL:
        prompt = (
            "Write a concise Markdown changelog entry for the PlusOS application "
            "'%s' in version %s. Short intro sentence then a bullet list of "
            "user-facing changes. Be specific to the notes/diff; don't invent "
            "features.\n%s%s" % (
                name, version,
                ("Notes:\n%s\n" % notes) if notes else "",
                ("Diffstat:\n%s\n" % diffstat) if diffstat else ""))
        body = json.dumps({"model": AI_MODEL, "max_tokens": 320, "temperature": 0.5,
                           "messages": [
                               {"role": "system", "content":
                                "You write short, accurate changelogs for PlusOS, a 68k Mac OS."},
                               {"role": "user", "content": prompt}]}).encode()
        try:
            req = urllib.request.Request(AI_BASE_URL.rstrip("/") + "/chat/completions",
                                         data=body, method="POST",
                                         headers={"Content-Type": "application/json",
                                                  "Authorization": "Bearer " + AI_API_KEY})
            with urllib.request.urlopen(req, timeout=120) as r:
                md = json.loads(r.read())["choices"][0]["message"]["content"].strip()
            if not md.lstrip().startswith("#"):
                md = "# %s — %s\n\n%s" % (name, version, md)
            return md
        except Exception as e:
            print("  (ai endpoint failed for %s: %s; using template)" % (name, e))
    body = notes.strip() or ("Rebuilt for PlusOS %s." % version)
    return ("# %s — %s\n\n%s\n\n- Recompiled against the latest SDK\n"
            "- Bundled in release %s\n" % (name, version, body, version))


def derive_version(server, channel, from_git) -> str:
    if from_git:
        try:
            v = subprocess.check_output(["git", "describe", "--tags", "--always", "--dirty"],
                                        cwd=REPO, text=True).strip()
            if v:
                return v
        except Exception:
            pass
    today = datetime.datetime.now(datetime.timezone.utc).strftime("%Y.%m.%d")
    # find how many releases already exist today on this channel
    status, body = _req("GET", server + "/api/catalog?channel=" + channel)
    n = 1
    if status == 200:
        existing = [r["version"] for r in json.loads(body).get("releases", [])]
        n = 1 + sum(1 for v in existing if v.startswith(today))
    return "%s-%d" % (today, n)


def main(argv=None):
    ap = argparse.ArgumentParser(description="Publish a PlusOS OTA release.")
    ap.add_argument("--server", default=os.environ.get("OTA_SERVER", "https://ota.d0a.net"),
                    help="base URL of the OTA server (default $OTA_SERVER or https://ota.d0a.net)")
    ap.add_argument("--channel", default="stable", choices=["stable", "beta", "nightly"])
    ap.add_argument("--machine", default=os.environ.get("MACHINE", "se"))
    ap.add_argument("--build", default=os.path.join(REPO, "build_p68"),
                    help="build output dir to scan (default: build_p68)")
    ap.add_argument("--version", help="explicit version (default: auto YYYY.MM.DD-n)")
    ap.add_argument("--version-from-git", action="store_true", help="use `git describe` for the version")
    ap.add_argument("--title", default="", help="short human title of this update")
    ap.add_argument("--notes", default="", help="release notes text")
    ap.add_argument("--notes-file", help="read release notes from a file")
    ap.add_argument("--repo", action="append", default=[], metavar="DIR_OR_FILE",
                    help="extra repository to include (repeatable); a dir is tarballed")
    ap.add_argument("--docs-dir", default=os.path.join(REPO, "build_p68", "docs", "apps"),
                    help="write per-app Markdown changelogs here (default: build_p68/docs/apps)")
    ap.add_argument("--no-docs", action="store_true", help="skip per-app Markdown generation")
    ap.add_argument("--dry-run", action="store_true", help="discover + hash, but don't upload/publish")
    a = ap.parse_args(argv)

    server = a.server.rstrip("/")
    key = load_key()
    arts = discover(a.build, a.repo)
    if not arts:
        sys.exit("publish_cli: no build artifacts found under %s (did you compile?)" % a.build)

    notes = a.notes
    if a.notes_file and os.path.isfile(a.notes_file):
        with open(a.notes_file, "r", encoding="utf-8") as fh:
            notes = fh.read()

    version = a.version or derive_version(server, a.channel, a.version_from_git)

    # Per-app Markdown changelogs, generated at compile time and written to disk
    # (one file per application, as requested) plus attached to the artifact so
    # the server stores exactly what we generated.
    app_docs = {}
    if not a.no_docs:
        diffstat = git_diffstat()
        os.makedirs(a.docs_dir, exist_ok=True)
        for art in arts:
            if art["kind"] != "app":
                continue
            md = ai_app_doc(art["name"], version, notes, diffstat)
            app_docs[art["name"]] = md
            with open(os.path.join(a.docs_dir, art["name"] + ".md"), "w", encoding="utf-8") as fh:
                fh.write(md + "\n")
        if app_docs:
            print("  wrote %d app changelog(s) to %s" % (len(app_docs), a.docs_dir))

    # hash everything
    print("PlusOS OTA publish -> %s  [%s/%s machine=%s]" % (server, a.channel, version, a.machine))
    manifest_arts = []
    for art in arts:
        sha = sha256_file(art["path_src"])
        size = os.path.getsize(art["path_src"])
        art["sha256"] = sha
        info = {"size": size}
        if art["name"] in app_docs:
            info["app_doc"] = app_docs[art["name"]]
        manifest_arts.append({"kind": art["kind"], "name": art["name"],
                              "path": art["dest"], "sha256": sha, "info": info})
        print("  %-10s %-18s %9d  %s" % (art["kind"], art["name"], size, sha[:12]))

    if a.dry_run:
        print("dry-run: %d artifact(s); nothing uploaded." % len(arts))
        return 0

    # upload missing blobs
    uploaded = skipped = 0
    for art in arts:
        if server_has(server, art["sha256"]):
            skipped += 1
            continue
        upload_blob(server, key, art["path_src"], art["sha256"])
        uploaded += 1
        print("  uploaded %s (%s)" % (art["name"], art["sha256"][:12]))
    print("  blobs: %d uploaded, %d already on server" % (uploaded, skipped))

    manifest = {"channel": a.channel, "version": version, "machine": a.machine,
                "title": a.title, "notes": notes, "artifacts": manifest_arts}
    result = publish_manifest(server, key, manifest)
    print("Published release #%d: %s/%s (%d artifacts)"
          % (result["release_id"], result["channel"], result["version"], result["artifacts"]))
    return 0


if __name__ == "__main__":
    sys.exit(main())

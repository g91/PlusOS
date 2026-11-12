"""Shared helpers for the hdbuild tests: a fake PlusOS build tree, oracles."""
import os
import shutil
import struct
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
HDBUILD = os.path.dirname(HERE)
REPO = os.path.dirname(os.path.dirname(HDBUILD))
if HDBUILD not in sys.path:
    sys.path.insert(0, HDBUILD)

from hdlib import forks, manifest  # noqa: E402


def fake_pex(seed, size):
    """A deterministic pseudo .pex: 'PLOS' at offset 4, length at offset 12."""
    body = bytearray(size)
    x = seed * 2654435761 & 0xFFFFFFFF
    for i in range(size):
        x = (x * 1103515245 + 12345) & 0xFFFFFFFF
        body[i] = (x >> 16) & 0xFF
    if size >= 16:
        body[0:4] = b"\x60\x00\x00\x2e"
        body[4:8] = b"PLOS"
        struct.pack_into(">I", body, 12, size)
    return bytes(body)


def fake_loader(kernel_block):
    return (b"LK\x60\x00\x00\x8a\x44\x18" + b"\0" * 120
            + struct.pack(">I", kernel_block * 512) + b"\x4e\x75" * 20)


APPS = ["shell", "desktop", "installer", "files", "cat", "ls", "calc", "tetris", "snake", "hello", "netdbg"]


def make_fake_repo(tmp, apps=APPS, cfg=True, floppy_loader=True, include=None):
    """Create <tmp>/repo with build/, apps/, multiboot.cfg and a Return app."""
    repo = os.path.join(tmp, "repo")
    os.makedirs(os.path.join(repo, "build", "boot"))
    os.makedirs(os.path.join(repo, "build", "kernel"))
    os.makedirs(os.path.join(repo, "build", "apps"))
    with open(os.path.join(repo, "build", "boot", "loader.bin"), "wb") as f:
        f.write(fake_loader(manifest.KERNEL_BLOCK))
    if floppy_loader:
        with open(os.path.join(repo, "build", "boot", "loader_floppy.bin"), "wb") as f:
            f.write(fake_loader(manifest.FLOPPY_KERNEL_BLOCK))
    with open(os.path.join(repo, "build", "kernel", "kernel.pex"), "wb") as f:
        f.write(fake_pex(1, 70000))
    for i, name in enumerate(apps):
        os.makedirs(os.path.join(repo, "apps", "group", name), exist_ok=True)
        with open(os.path.join(repo, "build", "apps", name + ".pex"), "wb") as f:
            f.write(fake_pex(100 + i, 300 + 1777 * i))
        with open(os.path.join(repo, "build", "apps", name + ".elf"), "wb") as f:
            f.write(b"not an app")
    if cfg:
        lines = ["# test config", "boot_pref=2"]
        if include is not None:
            lines.append("include_apps=" + ",".join(include))
        lines += ["", "[entry]", "name=PlusOS Shell", "id=shell", ""]
        with open(os.path.join(repo, "multiboot.cfg"), "wb") as f:
            f.write("\r\n".join(lines).encode("ascii"))
    os.makedirs(os.path.join(repo, "tools", "return_to_plusos"))
    with open(os.path.join(repo, "tools", "return_to_plusos", "Return to PlusOS.bin"), "wb") as f:
        f.write(forks.encode_macbinary(b"Return to PlusOS", b"", fake_pex(7, 851), b"APPL", b"PLOS",
                                       flags=0x2000, created=3877000000, modified=3877000100))
    return repo


def stage_opts(repo, **kw):
    o = {"repo": repo, "build": os.path.join(repo, "build"), "root": os.path.join(repo, "build", "disk_root"),
         "config": os.path.join(repo, "multiboot.cfg"),
         "return_app": os.path.join(repo, "tools", "return_to_plusos", "Return to PlusOS.bin")}
    o.update(kw)
    return o


def quiet(*a, **k):
    pass


def tree_snapshot(root):
    """{posix relpath: bytes or None for folders} of a staged folder (incl. sidecars, no manifest)."""
    out = {}
    for cur, dirs, files in os.walk(root):
        rel = os.path.relpath(cur, root).replace("\\", "/")
        rel = "" if rel == "." else rel + "/"
        for d in dirs:
            out[rel + d] = None
        for f in files:
            if rel == "" and f == manifest.MANIFEST_NAME:
                continue
            with open(os.path.join(cur, f), "rb") as fh:
                out[rel + f] = fh.read()
    return out


class TempDirMixin:
    def setUp(self):
        self.tmp = tempfile.mkdtemp(prefix="hdbuild_test_")

    def tearDown(self):
        from hdlib.stage import remove_tree
        try:
            remove_tree(self.tmp)
        except OSError:
            shutil.rmtree(self.tmp, ignore_errors=True)


# ---- oracles ------------------------------------------------------------------

_HFSUTILS = None


def hfsutils_runner():
    """-> (run(script_bytes) -> CompletedProcess, to_oracle_path(host_path)) or None.

    Uses native hfsutils when `hmount` is on PATH, else hfsutils inside WSL
    (distro from $HDBUILD_WSL_DISTRO, default "Ubuntu")."""
    global _HFSUTILS
    if _HFSUTILS is not None:
        return _HFSUTILS or None
    result = False
    if shutil.which("hmount"):
        def run(script_path):
            return subprocess.run(["sh", script_path], capture_output=True, timeout=600)
        result = (run, lambda p: p)
    elif os.name == "nt" and shutil.which("wsl"):
        distro = os.environ.get("HDBUILD_WSL_DISTRO", "Ubuntu")
        try:
            r = subprocess.run(["wsl", "-d", distro, "-e", "sh", "-c", "command -v hmount hcopy hls"],
                               capture_output=True, timeout=120)
            ok = r.returncode == 0 and b"hmount" in r.stdout
        except (OSError, subprocess.SubprocessError):
            ok = False
        if ok:
            def to_wsl(p):
                p = os.path.abspath(p)
                return "/mnt/" + p[0].lower() + p[2:].replace("\\", "/")

            def run(script_path):
                return subprocess.run(["wsl", "-d", distro, "-e", "sh", to_wsl(script_path)],
                                      capture_output=True, timeout=600)
            result = (run, to_wsl)
    _HFSUTILS = result
    return result or None


def sh_quote(raw):
    """Single-quote bytes for sh."""
    return b"'" + raw.replace(b"'", b"'" + bytes([0x5C]) + b"''") + b"'"


def hfs_escape(name):
    """Escape hfsutils glob characters in one path component (bytes)."""
    out = bytearray()
    for b in name:
        if b in b"*?[]{}," or b == 0x5C:
            out += b"\\"
        out.append(b)
    return bytes(out)

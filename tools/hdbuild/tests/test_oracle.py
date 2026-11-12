"""OPTIONAL cross-checks against independent HFS implementations.

    hfsutils   native `hmount` on PATH, or inside WSL (distro $HDBUILD_WSL_DISTRO,
               default "Ubuntu"): mount the image, compare `hls -lR`, fetch every
               file by name with `hcopy -r` / `hcopy -m` and compare bytes,
               type/creator and resource forks with the staged tree.
    machfs     the pip package `machfs`, if importable (pure-Python reader).
    DiskStudio the repo's C++ reader (PlusOSDiskStudio_linux, native or via WSL).

Every test skips cleanly when its oracle is not available.
"""
import os
import re
import shutil
import subprocess
import unittest

from . import util
from hdlib import forks, image, macroman, manifest, stage


def read(path):
    with open(path, "rb") as f:
        return f.read()


def write(path, data):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "wb") as f:
        f.write(data)


def odd_names():
    """One-character names for every MacRoman byte (minus HFS-equal duplicates)."""
    seen, out = set(), []
    for b in range(0x20, 0x100):
        if b == 0x3A:
            continue
        k = macroman.sort_key(bytes([b]))
        if k not in seen:
            seen.add(k)
            out.append(bytes([b]))
    return out


def populate(root):
    """Add awkward content to a staged folder."""
    for raw in odd_names():
        write(os.path.join(root, "Odd Names", "".join("%%%02X" % c for c in raw)), raw * 3)
    for name in ("Résumé", "Ärger.txt", "naïve file", "under_score", "[bracket]", "{brace}",
                 "back`tick", "tilde~", "it's", "100%25", "a%2Fb", "UPPER", "lower2", "Mixed Case Name",
                 "x" * 31, "sp ace  s", "dot.dot.dot", "æon", "Øre", "œuvre", "ßharp"):
        write(os.path.join(root, "Names", name), name.encode("utf-8") * 7)
    for i in range(260):                       # enough records for a multi-level index
        write(os.path.join(root, "Many", "d%02d" % (i % 13), "file-%04d.dat" % i), util.fake_pex(i, 37 * i % 3000))
    write(os.path.join(root, "Deep", "a", "b", "c", "d", "e", "leaf.txt"), b"leaf\r")
    write(os.path.join(root, "empty file"), b"")
    write(os.path.join(root, "Forked"), b"data fork of forked")
    write(os.path.join(root, ".rsrc", "Forked.rsrc"), util.fake_pex(3, 5000))
    man = manifest.load(root)
    man["entries"]["Forked"] = {"type": "APPL", "creator": "FRKD", "flags": "0x2000"}
    manifest.save(root, man)


def expected_files(root):
    """{tuple of MacRoman path components: hfs.File} of the staged folder."""
    man = manifest.load(root)
    top, _ = image.scan_tree(root, man)
    out, ndirs = {}, [0]

    def walk(items, prefix):
        for it in items:
            if it.is_dir:
                ndirs[0] += 1
                walk(it.children, prefix + (it.name,))
            else:
                out[prefix + (it.name,)] = it
    walk(top, ())
    return out, ndirs[0]


def src_bytes(src):
    return src if isinstance(src, (bytes, bytearray)) else read(src)


class Oracle(util.TempDirMixin, unittest.TestCase):
    def make(self, fmt):
        repo = util.make_fake_repo(self.tmp)
        if fmt == "floppy":
            root = os.path.join(self.tmp, "froot")
            stage.stage(util.stage_opts(repo, preset="floppy1440", root=root), log=util.quiet)
            write(os.path.join(root, "Names", "Résumé"), b"cv")
            out = os.path.join(self.tmp, "o.dsk")
            opts = {}
        else:
            root = os.path.join(repo, "build", "disk_root")
            stage.stage(util.stage_opts(repo), log=util.quiet)
            populate(root)
            out = os.path.join(self.tmp, "o.hda")
            opts = {"size": "64M" if fmt == "hda64" else "32M"}
        info = image.build_image(root, out, opts, log=util.quiet)
        return root, out, info


class Hfsutils(Oracle):
    def setUp(self):
        Oracle.setUp(self)
        self.runner = util.hfsutils_runner()
        if not self.runner:
            self.skipTest("hfsutils not available (neither native nor in WSL)")

    def check(self, fmt):
        run, opath = self.runner
        root, out, info = self.make(fmt)
        files, ndirs = expected_files(root)
        outdir = os.path.join(self.tmp, "oracle_out")
        os.makedirs(outdir)
        order = sorted(files)
        q = util.sh_quote
        lines = [b"export LC_ALL=C",
                 b"hmount " + q(opath(out).encode()) + b" > " + q(opath(os.path.join(outdir, "mount.txt")).encode())
                 + b" 2>&1 || exit 3",
                 b"hls -lR > " + q(opath(os.path.join(outdir, "listing.txt")).encode()) + b" 2>&1"]
        for i, path in enumerate(order):
            hpath = b":" + b":".join(util.hfs_escape(c) for c in path)
            for flag, ext in ((b"-r", "d"), (b"-m", "m")):
                dst = opath(os.path.join(outdir, "%d.%s" % (i, ext))).encode()
                lines.append(b"hcopy " + flag + b" " + q(hpath) + b" " + q(dst) + b" || echo FAIL " + flag
                             + b" %d" % i)
        lines.append(b"humount")
        script = os.path.join(self.tmp, "oracle.sh")
        with open(script, "wb") as f:
            f.write(b"\n".join(lines) + b"\n")
        r = run(script)
        self.assertEqual(r.returncode, 0, r.stdout + r.stderr)
        self.assertNotIn(b"FAIL", r.stdout, r.stdout[-2000:] + r.stderr[-2000:])
        self.assertIn(("Volume name is \"%s\"" % info["volume"]).encode(), read(os.path.join(outdir, "mount.txt")))
        self.assertIn(b"%d bytes free" % (info["free_blocks"] * info["alloc_block_size"]),
                      read(os.path.join(outdir, "mount.txt")))

        # every file: bytes, type/creator, resource fork
        for i, path in enumerate(order):
            f = files[path]
            data, rsrc = src_bytes(f.data), src_bytes(f.rsrc) if f.rsrc else b""
            self.assertEqual(read(os.path.join(outdir, "%d.d" % i)), data, path)
            mb = forks.decode_macbinary(read(os.path.join(outdir, "%d.m" % i)))
            self.assertIsNotNone(mb, path)
            self.assertEqual((mb.data, mb.rsrc), (data, rsrc), path)
            self.assertEqual((mb.ftype, mb.creator), (f.ftype, f.creator), path)
            self.assertEqual(mb.name, path[-1], path)

        # the recursive listing: same folders, same files, same sizes and codes
        listing = read(os.path.join(outdir, "listing.txt")).split(b"\n")
        cur, got, gdirs = (), {}, 0
        frx = re.compile(rb"^f[ i] (.{4})/(.{4}) +(\d+) +(\d+) .{12} (.*)$")
        drx = re.compile(rb"^d[ i] +(\d+) items? +.{12} (.*)$")
        for line in listing:
            if line.startswith(b":") and line.endswith(b":"):
                cur = tuple(line[1:-1].split(b":"))
                continue
            m = frx.match(line)
            if m:
                got[cur + (m.group(5),)] = (m.group(1), m.group(2), int(m.group(3)), int(m.group(4)))
            elif drx.match(line):
                gdirs += 1
        self.assertEqual(gdirs, ndirs)
        self.assertEqual(len(got), len(files))
        checked = 0
        for path, f in files.items():
            if all(all(32 < b < 127 for b in c.replace(b" ", b"x")) for c in path) and path in got:
                data, rsrc = src_bytes(f.data), src_bytes(f.rsrc) if f.rsrc else b""
                self.assertEqual(got[path], (f.ftype, f.creator, len(rsrc), len(data)), path)
                checked += 1
        self.assertGreater(checked, len(files) // 2)
        return info

    def test_hard_disk_32m(self):
        info = self.check("hda32")
        self.assertEqual(info["alloc_block_size"], 512)
        self.assertGreaterEqual(info["catalog"]["depth"], 3)

    def test_hard_disk_64m(self):
        info = self.check("hda64")
        self.assertEqual(info["alloc_block_size"], 1024)

    def test_floppy(self):
        self.check("floppy")

    def test_hfsutils_can_write_to_our_volume(self):
        """The catalog must stay usable when another implementation inserts into it."""
        run, opath = self.runner
        root, out, info = self.make("hda32")
        payload = os.path.join(self.tmp, "payload.bin")
        write(payload, util.fake_pex(99, 70000))
        q = util.sh_quote
        lines = [b"hmount " + q(opath(out).encode()) + b" >/dev/null || exit 3", b"hmkdir :NewDir || exit 4"]
        for i in range(150):                                   # forces leaf splits + a deeper index
            lines.append(b"hcopy -r " + q(opath(payload).encode()) + b" " + q(b":NewDir:added %03d" % i)
                         + b" || exit 5")
        lines += [b"hdel ':bin:SHELL' || exit 6", b"hrename ':bin:CALC' ':bin:Zcalc' || exit 7", b"humount"]
        script = os.path.join(self.tmp, "w.sh")
        with open(script, "wb") as f:
            f.write(b"\n".join(lines) + b"\n")
        r = run(script)
        self.assertEqual(r.returncode, 0, r.stdout + r.stderr)
        from hdlib import fsck
        rep = fsck.verify(out, expect_raw=True)
        self.assertEqual(rep.errors, [])
        with image.Opened(out) as img:
            vol = img.volume()
            self.assertEqual(len(vol.listdir(vol.lookup("NewDir"))), 150)
            self.assertEqual(vol.read_fork(vol.lookup("NewDir/added 149")), util.fake_pex(99, 70000))
            self.assertIsNone(vol.lookup("bin/SHELL"))
            self.assertIsNotNone(vol.lookup("bin/Zcalc"))
            # nothing was placed on the raw kernel / app area
            raw = image.detect_raw(vol)
            self.assertEqual(raw["kernel"]["block"], 60000)
            self.assertEqual(len(raw["apps"]["entries"]), len(util.APPS))

    def test_order_table_matches_hfsutils_binary(self):
        if shutil.which("hmount"):
            blob = read(shutil.which("hmount"))
        else:
            distro = os.environ.get("HDBUILD_WSL_DISTRO", "Ubuntu")
            blob = subprocess.run(["wsl", "-d", distro, "-e", "sh", "-c", "cat \"$(command -v hmount)\""],
                                  capture_output=True, timeout=120).stdout
        if macroman.ORDER[:0x40] not in blob:
            self.skipTest("hfsutils binary does not embed the table (dynamically linked libhfs?)")
        self.assertIn(macroman.ORDER, blob)


class Machfs(Oracle):
    def setUp(self):
        Oracle.setUp(self)
        try:
            import machfs  # noqa: F401
        except Exception as e:  # ImportError, or incompatibilities with a new Python
            self.skipTest("machfs not importable: %s" % e)

    def check(self, fmt):
        import machfs
        root, out, info = self.make(fmt)
        files, ndirs = expected_files(root)
        # machfs deliberately drops root items named Desktop / Desktop DB / Desktop DF on read
        files = {p: f for p, f in files.items() if p[0] != b"Desktop"}
        ndirs -= 1
        raw = read(out)
        if fmt != "floppy":
            raw = raw[info["hfs_start"] * 512:(info["hfs_start"] + info["hfs_blocks"]) * 512]
        v = machfs.Volume()
        v.read(raw)                      # (machfs does not read the volume name back)
        got, gdirs = {}, [0]

        def walk(folder, prefix):
            for name, child in folder.items():
                p = prefix + (name.encode("mac_roman"),)
                if isinstance(child, machfs.Folder):
                    gdirs[0] += 1
                    walk(child, p)
                else:
                    got[p] = child
        walk(v, ())
        self.assertEqual(gdirs[0], ndirs)
        # machfs keeps children in a dict keyed by a simplified lower-casing that folds some
        # accented one-letter names together, so a few "Odd Names" entries shadow each other
        # there; everything it does return must match, and nothing else may be missing.
        self.assertLessEqual(set(got), set(files))
        missing = set(files) - set(got)
        self.assertTrue(all(p[0] == b"Odd Names" for p in missing), missing)
        self.assertLess(len(missing), 40)
        for path, f in files.items():
            if path in missing:
                continue
            g = got[path]
            self.assertEqual(g.data, src_bytes(f.data), path)
            self.assertEqual(g.rsrc, src_bytes(f.rsrc) if f.rsrc else b"", path)
            self.assertEqual((g.type, g.creator), (f.ftype, f.creator), path)
            self.assertEqual(g.flags, f.flags, path)

    def test_hard_disk(self):
        self.check("hda32")

    def test_hard_disk_64m(self):
        self.check("hda64")

    def test_floppy(self):
        self.check("floppy")


class DiskStudio(Oracle):
    """The repo's own C++ HFS reader (tools/disk_studio), Linux CLI build."""

    def setUp(self):
        Oracle.setUp(self)
        exe = os.path.join(util.REPO, "PlusOSDiskStudio_linux")
        if not os.path.isfile(exe):
            self.skipTest("PlusOSDiskStudio_linux not built")
        if os.name == "nt":
            if not shutil.which("wsl"):
                self.skipTest("needs WSL to run the Linux binary")
            distro = os.environ.get("HDBUILD_WSL_DISTRO", "Ubuntu")
            conv = lambda p: "/mnt/" + os.path.abspath(p)[0].lower() + os.path.abspath(p)[2:].replace("\\", "/")
            self.cmd = lambda *a: subprocess.run(["wsl", "-d", distro, "-e", conv(exe)] + [
                conv(x) if os.path.exists(x) else x for x in a], capture_output=True, timeout=300)
        else:
            self.cmd = lambda *a: subprocess.run([exe] + list(a), capture_output=True, timeout=300)
        try:
            if self.cmd("help").returncode not in (0, 1):
                self.skipTest("PlusOSDiskStudio_linux does not run here")
        except (OSError, subprocess.SubprocessError) as e:
            self.skipTest("PlusOSDiskStudio_linux does not run here: %s" % e)

    def test_info_and_ls(self):
        repo = util.make_fake_repo(self.tmp)
        root = os.path.join(repo, "build", "disk_root")
        stage.stage(util.stage_opts(repo), log=util.quiet)
        out = os.path.join(self.tmp, "ds.hda")
        info = image.build_image(root, out, {"size": "33M"}, log=util.quiet)
        files, ndirs = expected_files(root)
        r = self.cmd("info", out)
        text = re.sub(rb"\x1b\[[0-9;]*m", b"", r.stdout)
        self.assertIn(b"Apple_HFS", text, r.stdout + r.stderr)
        self.assertIn(b"HFS Volume Name: PlusOS", text)
        self.assertRegex(text, rb"Total Files:\s+%d\b" % len(files))
        self.assertRegex(text, rb"Total Folders:\s+%d\b" % ndirs)
        self.assertRegex(text, rb"Alloc Block:\s+%d bytes" % info["alloc_block_size"])
        r = self.cmd("ls", out)
        text = re.sub(rb"\x1b\[[0-9;]*m", b"", r.stdout)
        for path, f in files.items():
            self.assertIn(path[-1], text, path)
        self.assertEqual(text.count(b"[FILE]"), len(files))
        self.assertIn(b"Type:PLOS Creator:PLOS", text)
        self.assertIn(b"Type:APPL Creator:PLOS", text)


if __name__ == "__main__":
    unittest.main()

"""stage -> image -> extract -> image round trips, partition maps, presets, CLI."""
import importlib.util
import io
import json
import os
import shutil
import struct
import unittest
from contextlib import redirect_stdout, redirect_stderr

from . import util
from hdlib import apm, cli, forks, fsck, image, manifest, stage
from hdlib.image import BuildError


def read(path):
    with open(path, "rb") as f:
        return f.read()


def write(path, data):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "wb") as f:
        f.write(data)


class Base(util.TempDirMixin, unittest.TestCase):
    def stage(self, **kw):
        repo = kw.pop("repo", None) or util.make_fake_repo(self.tmp)
        self.repo = repo
        opts = util.stage_opts(repo, **kw)
        stage.stage(opts, log=util.quiet)
        return opts["root"]

    def build(self, root, name="out.hda", **opts):
        out = os.path.join(self.tmp, name)
        opts.setdefault("size", "32M")
        info = image.build_image(root, out, opts, log=util.quiet)
        return out, info

    def assertVerifies(self, path, **kw):
        rep = fsck.verify(path, **kw)
        self.assertEqual(rep.errors, [], path)
        self.assertEqual(rep.warnings, [], path)
        return rep


class Staging(Base):
    def test_layout(self):
        root = self.stage()
        names = sorted(os.listdir(os.path.join(root, "bin")))
        self.assertEqual(names, sorted(a.upper() for a in util.APPS))
        self.assertEqual(sorted(os.listdir(os.path.join(root, "Games"))), ["SNAKE", "TETRIS"])
        self.assertEqual(read(os.path.join(root, "bin", "SHELL")),
                         read(os.path.join(self.repo, "build", "apps", "shell.pex")))
        self.assertEqual(read(os.path.join(root, "multiboot.cfg")), read(os.path.join(self.repo, "multiboot.cfg")))
        self.assertEqual(read(os.path.join(root, "Desktop", "Return to PlusOS")), b"")
        self.assertEqual(read(os.path.join(root, ".rsrc", "Desktop", "Return to PlusOS.rsrc")), util.fake_pex(7, 851))
        man = manifest.load(root, required=True)
        ent = man["entries"]["Desktop/Return to PlusOS"]
        self.assertEqual((ent["type"], ent["creator"], ent["flags"]), ("APPL", "PLOS", "0x2000"))
        self.assertEqual([e["name"] for e in man["raw"]["apps"]["entries"]], sorted(a.upper() for a in util.APPS))
        self.assertEqual(man["raw"]["kernel"]["block"], 60000)
        self.assertTrue(os.path.isfile(os.path.join(root, ".boot", "loader.bin")))
        self.assertTrue(os.path.isfile(os.path.join(root, ".boot", "kernel.pex")))

    def test_include_filter_like_install_py(self):
        repo = util.make_fake_repo(self.tmp, include=["calc", "TETRIS"])
        # a compiled program without a source folder under apps/ is "newly discovered": always kept
        write(os.path.join(repo, "build", "apps", "brandnew.pex"), util.fake_pex(55, 400))
        root = self.stage(repo=repo)
        self.assertEqual(sorted(os.listdir(os.path.join(root, "bin"))),
                         ["BRANDNEW", "CALC", "NETDBG", "SHELL", "TETRIS"])
        self.assertEqual(os.listdir(os.path.join(root, "Games")), ["TETRIS"])

    def test_custom_apps_and_seeded_cfg(self):
        repo = util.make_fake_repo(self.tmp, cfg=False)
        custom = os.path.join(self.tmp, "mine.pex")
        write(custom, util.fake_pex(9, 999))
        rel = os.path.relpath(custom, repo).replace("\\", "/")
        write(os.path.join(repo, "multiboot.cfg"), ("boot_pref=2\ncustom_apps=MINE:%s\n" % rel).encode())
        root = self.stage(repo=repo)
        self.assertEqual(read(os.path.join(root, "bin", "MINE")), util.fake_pex(9, 999))
        os.unlink(os.path.join(repo, "multiboot.cfg"))
        root = self.stage(repo=repo)
        self.assertEqual(read(os.path.join(root, "multiboot.cfg")), stage.DEFAULT_CFG.encode())
        self.assertIn(b"\r\n", read(os.path.join(root, "multiboot.cfg")))

    def test_cfg_eol(self):
        root = self.stage(cfg_eol="cr")
        data = read(os.path.join(root, "multiboot.cfg"))
        self.assertNotIn(b"\n", data)
        self.assertIn(b"\r", data)

    def test_refuses_to_wipe_foreign_folder(self):
        repo = util.make_fake_repo(self.tmp)
        root = os.path.join(self.tmp, "precious")
        write(os.path.join(root, "thesis.txt"), b"x")
        with self.assertRaises(stage.StageError):
            stage.stage(util.stage_opts(repo, root=root), log=util.quiet)
        self.assertTrue(os.path.exists(os.path.join(root, "thesis.txt")))

    def test_restage_over_read_only_tree(self):
        import stat
        root = self.stage()
        write(os.path.join(root, "stale file"), b"x")
        for cur, dirs, files in os.walk(root):
            for n in files + dirs:
                os.chmod(os.path.join(cur, n), stat.S_IREAD | stat.S_IEXEC)
        stage.stage(util.stage_opts(self.repo), log=util.quiet)
        self.assertFalse(os.path.exists(os.path.join(root, "stale file")))
        self.assertTrue(os.path.exists(os.path.join(root, "bin", "SHELL")))

    def test_overlay(self):
        ov = os.path.join(self.tmp, "overlay")
        write(os.path.join(ov, "Read Me.txt"), b"hello\r")
        write(os.path.join(ov, "Docs", "Résumé"), b"cv")
        write(os.path.join(ov, "Apps", "Tool.bin"),
              forks.encode_macbinary(b"Tool", b"DATA", b"RSRC" * 50, b"APPL", b"TOOL", flags=0x2100,
                                     created=3000000000, modified=3000000500))
        write(os.path.join(ov, "Apps", "notmacbinary.bin"), b"\0" * 300)
        write(os.path.join(ov, "Apps", "Doubled"), b"data fork")
        write(os.path.join(ov, "Apps", "._Doubled"), forks.encode_appledouble(b"double rsrc", b"TEXT", b"R*ch", 0x4000))
        write(os.path.join(ov, "bin", "SHELL"), b"replaced shell")
        write(os.path.join(ov, ".hdbuild.json"),
              json.dumps({"entries": {"Docs/Résumé": {"type": "WDBN", "creator": "MSWD"}}}).encode())
        root = self.stage(overlay=[ov])
        self.assertEqual(read(os.path.join(root, "bin", "SHELL")), b"replaced shell")
        self.assertEqual(read(os.path.join(root, "Apps", "Tool")), b"DATA")
        self.assertEqual(read(os.path.join(root, ".rsrc", "Apps", "Tool.rsrc")), b"RSRC" * 50)
        self.assertFalse(os.path.exists(os.path.join(root, "Apps", "Tool.bin")))
        self.assertTrue(os.path.exists(os.path.join(root, "Apps", "notmacbinary.bin")))
        man = manifest.load(root)
        self.assertEqual(man["entries"]["Apps/Tool"]["type"], "APPL")
        self.assertEqual(man["entries"]["Apps/Tool"]["flags"], "0x2000")      # 'inited' bit cleared
        out, info = self.build(root)
        self.assertVerifies(out)
        with image.Opened(out) as img:
            vol = img.volume()
            t = vol.lookup("Apps/Tool")
            self.assertEqual((t.ftype, t.creator, t.flags), (b"APPL", b"TOOL", 0x2000))
            self.assertEqual((t.created, t.modified), (3000000000, 3000000500))
            self.assertEqual(vol.read_fork(t, True), b"RSRC" * 50)
            d = vol.lookup("Apps/Doubled")
            self.assertEqual((d.ftype, d.creator, d.flags), (b"TEXT", b"R*ch", 0x4000))
            self.assertEqual((vol.read_fork(d), vol.read_fork(d, True)), (b"data fork", b"double rsrc"))
            r = vol.lookup("Docs/Résumé")
            self.assertEqual((r.name, r.ftype, r.creator), (b"R\x8esum\x8e", b"WDBN", b"MSWD"))
            self.assertEqual(vol.lookup("Read Me.txt").ftype, b"TEXT")
            self.assertEqual(vol.lookup("Apps/notmacbinary.bin").ftype, b"????")
            self.assertEqual(vol.lookup("bin/CALC").ftype, b"PLOS")


class HardDisk(Base):
    def test_layout_matches_install_py_expectations(self):
        root = self.stage()
        out, info = self.build(root)
        self.assertVerifies(out, expect_raw=True)
        self.assertEqual(os.path.getsize(out), 32 * 1024 * 1024)
        self.assertEqual(info["alloc_block_size"], 512)
        raw = read(out)
        total = len(raw) // 512
        self.assertEqual(raw[:2], b"ER")
        self.assertEqual(struct.unpack_from(">HI", raw, 2), (512, total))
        with open(out, "rb") as f:
            info = apm.parse(f)
        types = [(p["type"], p["start"], p["blocks"]) for p in info["partitions"]]
        self.assertEqual(types, [("Apple_partition_map", 1, 63), ("Apple_Free", 64, 32),
                                 ("Apple_HFS", 96, total - 96)])
        base = 96 * 512
        self.assertEqual(raw[base:base + 2], b"LK")
        self.assertEqual(raw[base + 1024:base + 1026], b"BD")
        # raw kernel and app store exactly where install.py puts them
        kernel = read(os.path.join(self.repo, "build", "kernel", "kernel.pex"))
        koff = base + 60000 * 512
        self.assertEqual(raw[koff:koff + len(kernel)], kernel)
        doff = base + 61000 * 512
        self.assertEqual(raw[doff:doff + 4], b"PLDR")
        count = struct.unpack_from(">I", raw, doff + 4)[0]
        self.assertEqual(count, len(util.APPS))
        nxt = 61004
        for i, name in enumerate(sorted(a.upper() for a in util.APPS)):
            ename, start, length = struct.unpack_from(">16sII", raw, doff + 8 + 24 * i)
            self.assertEqual(ename.rstrip(b"\0").decode(), name)
            self.assertEqual(start, nxt)
            data = read(os.path.join(self.repo, "build", "apps", name.lower() + ".pex"))
            self.assertEqual(length, len(data))
            self.assertEqual(raw[base + start * 512:base + start * 512 + length], data)
            nxt += (length + 511) // 512

    def test_install_py_accepts_the_image(self):
        path = os.path.join(util.REPO, "tools", "install.py")
        if not os.path.exists(path):
            self.skipTest("tools/install.py not present")
        spec = importlib.util.spec_from_file_location("plusos_install", path)
        inst = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(inst)
        root = self.stage()
        for size in ("32M", "64M"):
            out, info = self.build(root, name="inst.hda", size=size)
            copy = out + ".copy"
            shutil.copyfile(out, copy)
            with open(copy, "r+b") as f:
                partno, pstart, pcount, pname = inst.find_hfs_partition(f)
                self.assertEqual((partno, pstart, pname), (3, 96, "MacOS"))
                self.assertEqual(pcount, os.path.getsize(copy) // 512 - 96)
                vol = inst.Volume(f, pstart, pcount)
                self.assertEqual(vol.name, "PlusOS")
                self.assertEqual(vol.al_blk_siz, info["alloc_block_size"])
                self.assertEqual(vol.free_bks, info["free_blocks"])
                # install.py's own reservation finds every block already reserved
                total, already, newly = vol.reserve(60000, 61000 + 3000 - 1)
                self.assertEqual(newly, 0)
                self.assertEqual(total, already)
            self.assertEqual(read(copy), read(out))          # reserve() changed nothing

    def test_roundtrip_identical(self):
        root = self.stage()
        write(os.path.join(root, "Docs", "a%2Fb"), b"slash name")
        out, info = self.build(root)
        self.assertVerifies(out)
        dest = os.path.join(self.tmp, "extracted")
        image.extract(out, dest, log=util.quiet)
        a, b = util.tree_snapshot(root), util.tree_snapshot(dest)
        a[".boot/loader.bin"] = a[".boot/loader.bin"].ljust(1024, b"\0")
        self.assertEqual(sorted(a), sorted(b))
        for k in a:
            self.assertEqual(a[k], b[k], k)
        man = manifest.load(dest, required=True)
        self.assertEqual(man["raw"]["kernel"], {"path": ".boot/kernel.pex", "block": 60000, "max_blocks": 512})
        self.assertEqual(man["raw"]["apps"]["reserve_blocks"], 3000)
        self.assertEqual(man["raw"]["apps"]["entries"][0], {"name": "CALC", "path": "bin/CALC"})
        self.assertEqual(man["entries"]["Desktop/Return to PlusOS"]["rsrc"], ".rsrc/Desktop/Return to PlusOS.rsrc")
        with image.Opened(out) as img:
            self.assertEqual(img.volume().lookup(":Docs:a/b").name, b"a/b")
        out2 = os.path.join(self.tmp, "again.hda")
        image.build_image(dest, out2, {}, log=util.quiet)
        self.assertEqual(read(out), read(out2))

    def test_roundtrip_64m_alloc_1024(self):
        root = self.stage()
        out, info = self.build(root, size="64M")
        self.assertEqual(info["alloc_block_size"], 1024)
        self.assertVerifies(out)
        dest = os.path.join(self.tmp, "x")
        image.extract(out, dest, log=util.quiet)
        out2 = os.path.join(self.tmp, "again.hda")
        image.build_image(dest, out2, {}, log=util.quiet)
        self.assertEqual(read(out), read(out2))

    def test_too_small(self):
        root = self.stage()
        with self.assertRaises(BuildError) as cm:
            self.build(root, size="16M")
        self.assertIn("60000", str(cm.exception))
        self.assertFalse(os.path.exists(os.path.join(self.tmp, "out.hda")))

    def test_kernel_checks(self):
        root = self.stage()
        write(os.path.join(root, ".boot", "kernel.pex"), b"\0" * 5000)
        with self.assertRaises(BuildError) as cm:
            self.build(root)
        self.assertIn("PLOS", str(cm.exception))
        write(os.path.join(root, ".boot", "kernel.pex"), util.fake_pex(1, 512 * 512 + 1))
        with self.assertRaises(BuildError) as cm:
            self.build(root)
        self.assertIn("KERNEL_BLOCKS", str(cm.exception))
        write(os.path.join(root, ".boot", "loader.bin"), b"LK" + b"\0" * 1100)
        with self.assertRaises(BuildError):
            self.build(root)

    def test_loader_offset_warning(self):
        root = self.stage()
        out, info = self.build(root, kernel_block=50000)
        self.assertTrue(any("kernel byte offset" in w for w in info["warnings"]))

    def test_manifest_edits(self):
        root = self.stage()
        man = manifest.load(root)
        man["entries"]["bin/CALC"] = {"type": "TEST", "creator": "0x01020304", "flags": ["invisible"],
                                      "created": "1999-12-31T23:59:59", "modified": "2001-02-03T04:05:06",
                                      "locked": True, "location": [10, 20]}
        man["volume"]["name"] = "Edited"
        man["volume"]["created"] = "1990-01-01T00:00:00"
        man["rules"].insert(0, {"match": "Games/*", "type": "GAME", "creator": "PLOS"})
        manifest.save(root, man)
        out, info = self.build(root)
        self.assertVerifies(out)
        with image.Opened(out) as img:
            vol = img.volume()
            self.assertEqual(vol.uname, "Edited")
            self.assertEqual(manifest.iso_from_mac(vol.mdb["drCrDate"]), "1990-01-01T00:00:00")
            f = vol.lookup("bin/calc")
            self.assertEqual((f.ftype, f.creator, f.flags, f.locked), (b"TEST", b"\1\2\3\4", 0x4000, True))
            self.assertEqual(manifest.iso_from_mac(f.created), "1999-12-31T23:59:59")
            self.assertEqual(manifest.iso_from_mac(f.modified), "2001-02-03T04:05:06")
            self.assertEqual(f.location, (10, 20))
            self.assertEqual(vol.lookup("Games/TETRIS").ftype, b"GAME")

    def test_blessed_folder_survives_as_a_path(self):
        root = self.stage()
        write(os.path.join(root, "System Folder", "System"), b"sys")
        man = manifest.load(root)
        man["volume"]["blessed"] = "System Folder"
        manifest.save(root, man)
        out, info = self.build(root)
        self.assertVerifies(out)
        with image.Opened(out) as img:
            vol = img.volume()
            cnid = vol.lookup("System Folder").cnid
            self.assertEqual(struct.unpack_from(">I", vol.mdb["drFndrInfo"])[0], cnid)
        dest = os.path.join(self.tmp, "x")
        image.extract(out, dest, log=util.quiet)
        man2 = manifest.load(dest)
        self.assertEqual(man2["volume"]["blessed"], "System Folder")
        # add a folder that sorts first: CNIDs shift, the blessing must follow the folder
        write(os.path.join(dest, "AAA first", "f"), b"x")
        out2 = os.path.join(self.tmp, "again.hda")
        image.build_image(dest, out2, {}, log=util.quiet)
        with image.Opened(out2) as img:
            vol = img.volume()
            self.assertNotEqual(vol.lookup("System Folder").cnid, cnid)
            self.assertEqual(struct.unpack_from(">I", vol.mdb["drFndrInfo"])[0], vol.lookup("System Folder").cnid)
        man["volume"]["blessed"] = "No Such Folder"
        manifest.save(root, man)
        with self.assertRaises(BuildError):
            self.build(root)

    def test_plain_folder_without_manifest(self):
        root = os.path.join(self.tmp, "plain")
        write(os.path.join(root, "hello.txt"), b"hi")
        out, info = self.build(root, size="4M")
        rep = fsck.verify(out)
        self.assertEqual(rep.errors, [])
        with image.Opened(out) as img:
            self.assertEqual(img.volume().lookup("hello.txt").ftype, b"TEXT")

    def test_unrepresentable_names(self):
        root = self.stage()
        write(os.path.join(root, "Docs", "žluť.txt"), b"x")
        out, info = self.build(root)
        self.assertTrue(any("MacRoman" in w for w in info["warnings"]))
        with image.Opened(out) as img:
            self.assertIsNotNone(img.volume().lookup("Docs/zlut.txt"))
        with self.assertRaises(BuildError):
            self.build(root, strict_names=True)
        write(os.path.join(root, "Docs", "y" * 40), b"x")
        with self.assertRaises(BuildError):
            self.build(root)


class Drivers(Base):
    def driver_blob(self):
        return bytes((i * 31 + 7) & 0xFF for i in range(9392))

    def test_checksum(self):
        self.assertEqual(apm.driver_checksum(b""), 0xFFFF)
        self.assertEqual(apm.driver_checksum(b"\x01"), 0x0002)
        self.assertEqual(apm.driver_checksum(b"\x01\x02"), 0x0008)
        self.assertEqual(apm.driver_checksum(b"\x80\x00" + b"\0" * 14), 0x0080)   # rotates through bit 15

    def test_embedded_driver(self):
        root = self.stage()
        drv = os.path.join(self.tmp, "driver.bin")
        write(drv, self.driver_blob())
        out, info = self.build(root, driver=drv)
        self.assertVerifies(out)
        raw = read(out)
        with open(out, "rb") as f:
            p = apm.parse(f)
        self.assertEqual(p["drivers"], [(64, 19, 1)])                    # ddBlock, ddSize (blocks), ddType
        d = apm.find_driver(p)
        self.assertEqual((d["type"], d["start"], d["blocks"], d["status"]), ("Apple_Driver43", 64, 32, 0x7F))
        self.assertEqual((d["boot_size"], d["processor"]), (9392, "68000"))
        self.assertEqual(d["boot_cksum"], apm.driver_checksum(self.driver_blob()))
        self.assertEqual(d["bootargs"], apm.DEFAULT_BOOTARGS)
        self.assertEqual(raw[64 * 512:64 * 512 + 9392], self.driver_blob())
        self.assertEqual([x["type"] for x in p["partitions"]],
                         ["Apple_partition_map", "Apple_Driver43", "Apple_HFS"])
        self.assertEqual(apm.find_hfs(p)["start"], 96)
        # extract-driver + extract + rebuild is byte-identical
        saved = os.path.join(self.tmp, "saved.drv")
        with redirect_stdout(io.StringIO()):
            self.assertEqual(cli.main(["extract-driver", out, saved]), 0)
        self.assertEqual(read(saved), self.driver_blob())
        dest = os.path.join(self.tmp, "x")
        image.extract(out, dest, log=util.quiet)
        out2 = os.path.join(self.tmp, "again.hda")
        image.build_image(dest, out2, {}, log=util.quiet)
        self.assertEqual(read(out), read(out2))
        out3 = os.path.join(self.tmp, "third.hda")
        image.build_image(root, out3, {"driver": saved, "size": "32M"}, log=util.quiet)
        self.assertEqual(read(out)[:96 * 512], read(out3)[:96 * 512])

    def test_bad_checksum_detected(self):
        root = self.stage()
        drv = os.path.join(self.tmp, "driver.bin")
        write(drv, self.driver_blob())
        out, info = self.build(root, driver=drv)
        raw = bytearray(read(out))
        raw[64 * 512 + 100] ^= 0xFF
        write(out, bytes(raw))
        rep = fsck.verify(out)
        self.assertTrue(any("checksum" in e for e in rep.errors))

    def test_from_template(self):
        root = self.stage()
        drv = os.path.join(self.tmp, "driver.bin")
        write(drv, self.driver_blob())
        template, _ = self.build(root, name="template.hda", driver=drv, volume_name="Old Volume")
        # scribble into the template's volume: only its first 96 blocks must be used
        write(os.path.join(root, "bin", "EXTRA"), util.fake_pex(77, 1234))
        out = os.path.join(self.tmp, "fromtpl.hda")
        info = image.build_image(root, out, {"from_template": template}, log=util.quiet)
        self.assertTrue(info["has_driver"])
        self.assertVerifies(out)
        self.assertEqual(os.path.getsize(out), os.path.getsize(template))
        self.assertEqual(read(out)[:96 * 512], read(template)[:96 * 512])
        with image.Opened(out) as img:
            vol = img.volume()
            self.assertEqual(vol.uname, "PlusOS")
            self.assertIsNotNone(vol.lookup("bin/EXTRA"))
        # resized
        out2 = os.path.join(self.tmp, "fromtpl40.hda")
        image.build_image(root, out2, {"from_template": template, "size": "40M"}, log=util.quiet)
        self.assertVerifies(out2)
        with open(out2, "rb") as f:
            p = apm.parse(f)
        self.assertEqual(p["block_count"], 40 * 2048)
        self.assertEqual(apm.find_hfs(p)["blocks"], 40 * 2048 - 96)
        self.assertEqual(apm.find_driver(p)["boot_cksum"], apm.driver_checksum(self.driver_blob()))
        with self.assertRaises(BuildError):
            image.build_image(root, out2, {"from_template": os.path.join(root, "bin", "SHELL")}, log=util.quiet)


class Floppy(Base):
    def test_preset_1440(self):
        repo = util.make_fake_repo(self.tmp)
        root = self.stage(repo=repo, preset="floppy1440", root=os.path.join(self.tmp, "froot"))
        self.assertEqual(sorted(os.listdir(os.path.join(root, "bin"))),
                         sorted(a.upper() for a in util.APPS if a.upper() in stage.FLOPPY_APPS))
        self.assertFalse(os.path.exists(os.path.join(root, "Games")))
        self.assertEqual(read(os.path.join(root, "multiboot.cfg")), stage.FLOPPY_CFG.encode())
        self.assertEqual(read(os.path.join(root, "Desktop", "Installer")),
                         read(os.path.join(repo, "build", "apps", "installer.pex")))
        out = os.path.join(self.tmp, "f.dsk")
        info = image.build_image(root, out, {}, log=util.quiet)
        self.assertEqual(info["warnings"], [])
        self.assertEqual(os.path.getsize(out), 1474560)
        self.assertVerifies(out, expect_raw=True)
        raw = read(out)
        self.assertEqual(raw[:2], b"LK")                       # no partition map: volume starts at block 0
        self.assertEqual(raw[1024:1026], b"BD")
        kernel = read(os.path.join(repo, "build", "kernel", "kernel.pex"))
        self.assertEqual(raw[100 * 512:100 * 512 + len(kernel)], kernel)
        self.assertNotIn(b"PLDR", raw[:4] + raw[61000 * 512:61000 * 512 + 4])
        with image.Opened(out) as img:
            vol = img.volume()
            self.assertEqual(vol.uname, "PlusOS Install")
            self.assertEqual(vol.lookup("Desktop/Installer").ftype, b"PLOS")
            self.assertEqual(vol.lookup("Desktop/Return to PlusOS").rlen, 851)
            bm = vol.bitmap()
            for sector in range(100, 460):                     # 360 blocks = KERNEL_BYTES 184320
                a = sector - vol.alst
                self.assertTrue(bm[a >> 3] & (0x80 >> (a & 7)))
            # no file may sit on the kernel
            for f in vol.files.values():
                for start, count in list(f.dext) + list(f.rext):
                    if count:
                        self.assertFalse(start + vol.alst < 460 and start + count + vol.alst > 100)
        dest = os.path.join(self.tmp, "fx")
        image.extract(out, dest, log=util.quiet)
        man = manifest.load(dest, required=True)
        self.assertEqual(man["image"]["format"], "dsk")
        self.assertEqual(man["raw"]["kernel"]["block"], 100)
        self.assertEqual(man["raw"]["kernel"]["max_blocks"], 360)
        self.assertIsNone(man["raw"]["apps"])
        out2 = os.path.join(self.tmp, "f2.dsk")
        image.build_image(dest, out2, {}, log=util.quiet)
        self.assertEqual(read(out), read(out2))

    def test_preset_800(self):
        repo = util.make_fake_repo(self.tmp)
        root = self.stage(repo=repo, preset="floppy800", root=os.path.join(self.tmp, "froot"))
        out = os.path.join(self.tmp, "f.dsk")
        image.build_image(root, out, {}, log=util.quiet)
        self.assertEqual(os.path.getsize(out), 819200)
        self.assertVerifies(out, expect_raw=True)

    def test_floppy_needs_floppy_loader(self):
        repo = util.make_fake_repo(self.tmp, floppy_loader=False)
        with self.assertRaises(stage.StageError) as cm:
            stage.stage(util.stage_opts(repo, preset="floppy1440", root=os.path.join(self.tmp, "froot")),
                        log=util.quiet)
        self.assertIn("loader_floppy.bin", str(cm.exception))

    def test_bare_dsk_any_size(self):
        root = self.stage(format="dsk", size="40M")
        out = os.path.join(self.tmp, "bare.dsk")
        info = image.build_image(root, out, {}, log=util.quiet)
        self.assertEqual(info["format"], "dsk")
        self.assertVerifies(out, expect_raw=True)
        self.assertEqual(read(out)[1024:1026], b"BD")


class CommandLine(Base):
    def run_cli(self, *argv):
        out, err = io.StringIO(), io.StringIO()
        with redirect_stdout(out), redirect_stderr(err):
            rc = cli.main(list(argv))
        return rc, out.getvalue(), err.getvalue()

    def test_build_ls_info_verify_extract(self):
        repo = util.make_fake_repo(self.tmp)
        root = os.path.join(repo, "build", "disk_root")
        img = os.path.join(self.tmp, "cli.hda")
        # --build/--config point into the fake repo; the Return app comes from the real repo if present
        rc, out, err = self.run_cli("build", "--build", os.path.join(repo, "build"), "--root", root,
                                    "--config", os.path.join(repo, "multiboot.cfg"), "--size", "32M",
                                    "--out", img, "--volume-name", "CLI Test")
        self.assertEqual(rc, 0, err + out)
        self.assertIn("verify: OK", out)
        self.assertIn("NO driver partition", out)
        rc, out, err = self.run_cli("verify", "--expect-raw", "--strict", img)
        self.assertEqual(rc, 0, out)
        rc, out, err = self.run_cli("ls", "-l", img, "bin")
        self.assertEqual(rc, 0)
        self.assertIn("PLOS/PLOS", out)
        self.assertIn("SHELL", out)
        rc, out, err = self.run_cli("ls", "-R", img)
        self.assertIn(":bin:", out)
        rc, out, err = self.run_cli("ls", img, ":bin:shell")
        self.assertEqual(out.strip(), "SHELL")
        rc, out, err = self.run_cli("ls", img, "nope")
        self.assertEqual(rc, 1)
        self.assertIn("no such file", err)
        rc, out, err = self.run_cli("info", "-v", img)
        self.assertEqual(rc, 0)
        for needle in ("Apple_HFS", "CLI Test", "kernel: block 60000", "app store: directory at block 61000"):
            self.assertIn(needle, out)
        dest = os.path.join(self.tmp, "cli_x")
        rc, out, err = self.run_cli("extract", img, dest)
        self.assertEqual(rc, 0, err)
        self.assertTrue(os.path.exists(os.path.join(dest, "bin", "SHELL")))
        rc, out, err = self.run_cli("extract", img, dest)
        self.assertEqual(rc, 1)                                 # refuses a non-empty folder
        rc, out, err = self.run_cli("image", "--root", dest, "--out", os.path.join(self.tmp, "cli2.hda"))
        self.assertEqual(rc, 0, err)
        self.assertEqual(read(img), read(os.path.join(self.tmp, "cli2.hda")))
        rc, out, err = self.run_cli("verify", os.path.join(repo, "multiboot.cfg"))
        self.assertEqual(rc, 1)

    def test_errors_are_clean(self):
        repo = util.make_fake_repo(self.tmp)
        rc, out, err = self.run_cli("build", "--build", os.path.join(repo, "build"),
                                    "--root", os.path.join(repo, "build", "disk_root"),
                                    "--config", os.path.join(repo, "multiboot.cfg"),
                                    "--size", "8M", "--out", os.path.join(self.tmp, "x.hda"))
        self.assertEqual(rc, 1)
        self.assertIn("hdbuild: error:", err)
        self.assertNotIn("Traceback", err)


if __name__ == "__main__":
    unittest.main()

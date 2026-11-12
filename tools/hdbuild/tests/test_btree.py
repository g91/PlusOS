"""The HFS formatter/writer: geometry, B*-trees under stress, allocation, fsck."""
import io
import struct
import unittest

from . import util  # noqa: F401
from hdlib import fsck, hfs, macroman
from hdlib.hfs import VolumeBuilder, Dir, File, HFSError, SECTOR
from hdlib.hfsread import Volume


def new_image(nsect):
    return io.BytesIO(bytes(nsect * SECTOR))


def check(buf, nsect, **kw):
    rep = fsck.Report()
    vol = fsck.check_volume(buf, 0, nsect, rep, **kw)
    return rep, vol


def content(i, n):
    return bytes((i * 7 + j * 13) & 0xFF for j in range(n))


class Geometry(unittest.TestCase):
    def test_known_layouts(self):
        # (sectors) -> (alloc block, bitmap sectors, alloc blocks, first alloc sector)
        self.assertEqual(hfs.geometry(2880), (512, 1, 2874, 4))          # 1.44M floppy (hformat)
        self.assertEqual(hfs.geometry(1600), (512, 1, 1594, 4))          # 800K floppy
        self.assertEqual(hfs.geometry(65438), (512, 16, 65417, 19))      # the 32 MB BlueSCSI image
        self.assertEqual(hfs.geometry(130976)[0], 1024)                  # 64 MB

    def test_alloc_blocks_fit_16_bits(self):
        for mb in (1, 20, 32, 33, 64, 100, 128, 255, 256, 500, 1024, 2047):
            nsect = mb * 2048
            alblk, vbm, nmal, alst = hfs.geometry(nsect)
            self.assertLessEqual(nmal, 65535)
            self.assertEqual(alblk % 512, 0)
            self.assertLessEqual(alst + nmal * (alblk // 512), nsect - 2)
            self.assertGreaterEqual(vbm * 4096, nmal)
            self.assertEqual(alst, 3 + vbm)

    def test_limits(self):
        with self.assertRaises(HFSError):
            hfs.geometry(2 * 1024 * 1024 * 2 + 10)       # > 2 GB
        with self.assertRaises(HFSError):
            hfs.geometry(10)
        with self.assertRaises(HFSError):
            hfs.geometry(130976, 512)                    # too many blocks for 16 bits
        self.assertEqual(hfs.geometry(2880, 1024)[0], 1024)


class EmptyVolume(unittest.TestCase):
    def test_empty(self):
        nsect = 2880
        buf = new_image(nsect)
        vb = VolumeBuilder(nsect, b"Empty", created=3000000000, modified=3000000001)
        info = vb.build(buf)
        rep, vol = check(buf, nsect)
        self.assertEqual(rep.errors, [])
        self.assertEqual(rep.warnings, [])
        self.assertEqual(vol.name, b"Empty")
        self.assertEqual(vol.catalog_tree.depth, 1)
        self.assertEqual(vol.catalog_tree.nrecs, 2)          # root folder + its thread
        self.assertEqual(vol.extents_tree.depth, 0)
        self.assertEqual(vol.mdb["drNxtCNID"], 16)
        self.assertEqual(vol.mdb["drFreeBks"], info["free_blocks"])
        self.assertEqual(vol.mdb["drCrDate"], 3000000000)
        raw = buf.getvalue()
        self.assertEqual(raw[1024:1024 + 162], raw[(nsect - 2) * 512:(nsect - 2) * 512 + 162])
        self.assertEqual(raw[1024:1026], b"BD")
        self.assertEqual(vol.mdb["drFndrInfo"], bytes(32))   # nothing blessed: boot blocks do the booting

    def test_volume_name_rules(self):
        with self.assertRaises(HFSError):
            VolumeBuilder(2880, b"x" * 28)
        with self.assertRaises(HFSError):
            VolumeBuilder(2880, b"a:b")
        with self.assertRaises(HFSError):
            VolumeBuilder(2880, b"")


class Stress(unittest.TestCase):
    def build(self, nsect, nfiles, per_dir=40, depth_every=7):
        buf = new_image(nsect)
        vb = VolumeBuilder(nsect, b"Stress Test", created=3000000000, modified=3000000000)
        expect = {}
        specials = [b"\x80rger", b"r\x8esum\x8e", b"_under", b"[br]", b"{br}", b"`tick", b"~tilde",
                    b"UPPER", b"lower", b"MiXeD", b"a/b", b"100%", b" lead", b"dot.", b"\xa7 sharp"]
        cur, path, dirs = vb.root, (), 0
        for i in range(nfiles):
            if i % per_dir == 0:
                dirs += 1
                # alternate: go deeper / start a new top-level branch
                name = ("Folder %05d" % i).encode()
                if dirs % depth_every == 0:
                    cur, path = vb.root, ()
                d = cur.add(Dir(name, created=3000000000 + i, modified=3000000000 + i))
                cur, path = d, path + (name,)
            base = specials[i % len(specials)] if (i // per_dir) % 3 == 0 and i % per_dir < len(specials) \
                else ("f%05d-%s" % (i, "x" * (i % 20))).encode()
            data = content(i, (i * 37) % 1500)
            rsrc = content(i + 1, 100) if i % 11 == 0 else b""
            cur.add(File(base, data=data, rsrc=rsrc, ftype=b"TEXT", creator=b"ttxt", flags=i & 0x4000,
                         created=3000000000 + i, modified=3100000000 + i))
            expect[path + (base,)] = (data, rsrc)
        info = vb.build(buf)
        return buf, info, expect, dirs

    def verify_all(self, buf, nsect, expect, dirs):
        rep, vol = check(buf, nsect)
        self.assertEqual(rep.errors, [])
        self.assertEqual(rep.warnings, [])
        got = {}
        ndirs = 0
        for path, e in vol.walk():
            if e.is_dir:
                ndirs += 1
            else:
                got[path] = (vol.read_fork(e), vol.read_fork(e, True))
        self.assertEqual(ndirs, dirs)
        self.assertEqual(set(got), set(expect))
        for k in expect:
            self.assertEqual(got[k], expect[k], k)
        return vol

    def test_thousands_of_files_multi_level_index(self):
        nsect = 20 * 2048
        buf, info, expect, dirs = self.build(nsect, 3000)
        vol = self.verify_all(buf, nsect, expect, dirs)
        self.assertGreaterEqual(vol.catalog_tree.depth, 4)
        self.assertEqual(vol.catalog_tree.nrecs, 3000 + 2 * (dirs + 1))
        self.assertEqual(vol.mdb["drFilCnt"], 3000)
        self.assertEqual(vol.mdb["drDirCnt"], dirs)
        # case-insensitive lookup through the reader
        self.assertIsNotNone(vol.lookup("folder 00000/ÄRGER"))
        self.assertIsNotNone(vol.lookup(":Folder 00000:upper"))
        self.assertIsNone(vol.lookup("Folder 00000/nope"))
        # leaves are filled and chained
        leaves = list(vol.catalog_tree.leaves())
        self.assertGreater(len(leaves), 500)
        self.assertEqual(leaves[0].blink, 0)
        self.assertEqual(leaves[-1].flink, 0)

    def test_catalog_needs_map_nodes(self):
        # > 2048 catalog nodes: the node bitmap continues in map nodes
        nsect = 24 * 2048
        buf, info, expect, dirs = self.build(nsect, 11000, per_dir=200, depth_every=3)
        self.assertGreaterEqual(info["catalog"]["map_nodes"], 1)
        self.assertGreater(info["catalog"]["used_nodes"], 2048)
        vol = self.verify_all(buf, nsect, expect, dirs)
        bits, maps = vol.catalog_tree.map_bits()
        self.assertEqual(len(maps), info["catalog"]["map_nodes"])

    def test_deep_nesting(self):
        nsect = 4096
        buf = new_image(nsect)
        vb = VolumeBuilder(nsect, b"Deep")
        cur = vb.root
        for i in range(60):
            cur = cur.add(Dir(("level %d" % i).encode()))
        cur.add(File(b"bottom", data=b"hello"))
        vb.build(buf)
        rep, vol = check(buf, nsect)
        self.assertEqual(rep.errors, [])
        e = vol.lookup("/".join("level %d" % i for i in range(60)) + "/bottom")
        self.assertEqual(vol.read_fork(e), b"hello")


class Allocation(unittest.TestCase):
    def test_reserved_area_and_split_extents(self):
        nsect = 2880
        buf = new_image(nsect)
        vb = VolumeBuilder(nsect, b"Floppy")
        a0, a1 = vb.reserve(1000, 1499)
        self.assertEqual((a0, a1), (996, 1495))              # (sector - drAlBlSt) / 1, as install.py
        vb.raw_write(1000, b"RAWDATA" * 100)
        big = content(1, 700 * 512)
        huge = content(2, 1400 * 512)                         # larger than any single free run
        vb.root.add(File(b"A big", data=big))
        vb.root.add(File(b"B huge", data=huge))
        vb.build(buf)
        rep, vol = check(buf, nsect)
        self.assertEqual(rep.errors, [])
        fa, fb = vol.lookup("A big"), vol.lookup("B huge")
        self.assertEqual(vol.read_fork(fa), big)
        self.assertEqual(vol.read_fork(fb), huge)
        self.assertGreaterEqual(len([e for e in fb.dext if e[1]]), 2)   # split around the raw area
        for f in (fa, fb):
            for start, count in f.dext:
                self.assertFalse(start <= 1495 and start + count > 996 and count, (start, count))
        raw = buf.getvalue()
        self.assertEqual(raw[1000 * 512:1000 * 512 + 7], b"RAWDATA")
        bm = vol.bitmap()
        for a in range(996, 1496):
            self.assertTrue(bm[a >> 3] & (0x80 >> (a & 7)))
        # those blocks are allocated but unowned -> reported as a warning (no PLOS kernel there)
        self.assertTrue(any("996-1495" in w for w in rep.warnings), rep.warnings)

    def test_reserve_outside_volume(self):
        vb = VolumeBuilder(2880, b"Floppy")
        with self.assertRaises(HFSError):
            vb.reserve(60000, 63999)

    def test_volume_full(self):
        nsect = 1600
        vb = VolumeBuilder(nsect, b"Full")
        vb.root.add(File(b"too big", data=bytes(800 * 1024)))
        with self.assertRaises(HFSError) as cm:
            vb.build(new_image(nsect))
        self.assertIn("volume full", str(cm.exception))

    def test_exact_fill(self):
        nsect = 1600
        probe = VolumeBuilder(nsect, b"Fill")
        free = probe.build(new_image(nsect))["free_blocks"]
        vb = VolumeBuilder(nsect, b"Fill")
        vb.root.add(File(b"all", data=content(3, free * 512)))
        buf = new_image(nsect)
        info = vb.build(buf)
        self.assertEqual(info["free_blocks"], 0)
        rep, vol = check(buf, nsect)
        self.assertEqual(rep.errors, [])
        self.assertEqual(vol.read_fork(vol.lookup("all")), content(3, free * 512))

    def test_duplicate_names_rejected(self):
        for a, b in ((b"Readme", b"README"), (b"\x80", b"\x8a")):      # A-umlaut == a-umlaut
            vb = VolumeBuilder(2880, b"Dup")
            vb.root.add(File(a))
            vb.root.add(File(b))
            with self.assertRaises(HFSError):
                vb.build(new_image(2880))

    def test_illegal_names_rejected(self):
        for bad in (b"", b"x" * 32, b"a:b"):
            vb = VolumeBuilder(2880, b"Bad")
            vb.root.add(File(bad))
            with self.assertRaises(HFSError):
                vb.build(new_image(2880))

    def test_metadata_roundtrip(self):
        nsect = 2880
        buf = new_image(nsect)
        vb = VolumeBuilder(nsect, b"Meta", finder_info=b"\0\0\0\x12")
        vb.root.add(File(b"App", data=b"d", rsrc=b"rsrc!", ftype=b"APPL", creator=b"PLOS", flags=0x2100,
                         location=(-5, 77), fxinfo=b"\1\2\3", created=111, modified=222, backup=333,
                         locked=True))
        vb.build(buf)
        rep, vol = check(buf, nsect)
        self.assertEqual(rep.errors, [])
        f = vol.lookup("app")
        self.assertEqual((f.ftype, f.creator, f.flags, f.location), (b"APPL", b"PLOS", 0x2100, (-5, 77)))
        self.assertEqual((f.created, f.modified, f.backup, f.locked), (111, 222, 333, True))
        self.assertEqual(f.fxinfo[:3], b"\1\2\3")
        self.assertEqual((vol.read_fork(f), vol.read_fork(f, True)), (b"d", b"rsrc!"))
        self.assertEqual(f.dphys, 512)
        self.assertEqual(vol.mdb["drFndrInfo"][:4], b"\0\0\0\x12")


class FsckDetects(unittest.TestCase):
    """verify must actually notice damage."""

    def setUp(self):
        self.nsect = 4096
        buf = new_image(self.nsect)
        vb = VolumeBuilder(self.nsect, b"Victim")
        d = vb.root.add(Dir(b"dir"))
        for i in range(120):
            (d if i % 2 else vb.root).add(File(("file %03d" % i).encode(), data=content(i, 700)))
        vb.build(buf)
        self.raw = bytearray(buf.getvalue())
        rep, self.vol = check(buf, self.nsect)
        self.assertEqual(rep.errors, [])

    def errors_after(self, mutate):
        raw = bytearray(self.raw)
        mutate(raw)
        rep, _ = check(io.BytesIO(bytes(raw)), self.nsect)
        return rep.errors

    def cat_off(self, node):
        start = self.vol.ct_ext[0][0]
        return (self.vol.alst + start * self.vol.per) * 512 + node * 512

    def test_bitmap_bit_cleared(self):
        def m(raw):
            raw[3 * 512 + 2] &= 0x7F
        self.assertTrue(any("free in the volume bitmap" in e for e in self.errors_after(m)))

    def test_free_count(self):
        def m(raw):
            struct.pack_into(">H", raw, 1024 + 34, 7)
        self.assertTrue(any("drFreeBks" in e for e in self.errors_after(m)))

    def test_alt_mdb_missing(self):
        def m(raw):
            raw[(self.nsect - 2) * 512:(self.nsect - 2) * 512 + 2] = b"\0\0"
        self.assertTrue(any("alternate MDB" in e for e in self.errors_after(m)))

    def test_leaf_link_broken(self):
        first = self.vol.catalog_tree.first_leaf

        def m(raw):
            struct.pack_into(">I", raw, self.cat_off(first), 0)           # fLink of the first leaf
        self.assertTrue(self.errors_after(m))

    def test_key_order(self):
        leaf = self.vol.catalog_tree.node(self.vol.catalog_tree.first_leaf)
        off = self.cat_off(leaf.num)

        def m(raw):                                                       # swap two record offsets
            a, b = 512 - 2, 512 - 4
            raw[off + a:off + a + 2], raw[off + b:off + b + 2] = raw[off + b:off + b + 2], raw[off + a:off + a + 2]
        self.assertTrue(self.errors_after(m))

    def test_valence(self):
        # root directory record is the first record of the first leaf: dirVal at key(8..)+4
        leaf = self.vol.catalog_tree.node(self.vol.catalog_tree.first_leaf)
        rec = leaf.recs[0]
        klen = rec[0] + 1 + ((rec[0] + 1) & 1)
        off = self.cat_off(leaf.num) + leaf.offsets[0] + klen + 4

        def m(raw):
            struct.pack_into(">H", raw, off, 3)
        self.assertTrue(any("valence" in e for e in self.errors_after(m)))

    def test_node_map(self):
        def m(raw):
            raw[self.cat_off(0) + 0xF8] = 0x80                            # only the header marked used
        self.assertTrue(any("node map" in e for e in self.errors_after(m)))

    def test_file_count(self):
        def m(raw):
            struct.pack_into(">I", raw, 1024 + 84, 5)
        self.assertTrue(any("drFilCnt" in e for e in self.errors_after(m)))

    def test_overlapping_extents(self):
        f = self.vol.lookup("file 000")
        g = self.vol.lookup("file 002")
        self.assertNotEqual(f.dext[0], g.dext[0])
        # find g's record and point its first extent at f's block
        for n in self.vol.catalog_tree.leaves():
            for i, rec in enumerate(n.recs):
                if b"file 002" in rec[:20] and rec[rec[0] + 1 + ((rec[0] + 1) & 1)] == 2:
                    off = self.cat_off(n.num) + n.offsets[i] + rec[0] + 1 + ((rec[0] + 1) & 1) + 74

                    def m(raw):
                        struct.pack_into(">H", raw, off, f.dext[0][0])
                    self.assertTrue(any("also used by" in e for e in self.errors_after(m)))
                    return
        self.fail("record not found")


if __name__ == "__main__":
    unittest.main()

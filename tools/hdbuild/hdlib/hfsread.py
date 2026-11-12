"""Read an HFS volume: MDB, B*-trees, catalog, forks (incl. overflow extents).

Works on volumes written by anything (hdbuild, hfsutils, Mac OS), not only on
our own output.
"""
import struct

from . import macroman
from .hfs import (SECTOR, NODE, MDB_FMT, MDB_FIELDS, MDB_SIZE, MDB_SIG, BTH_FMT, DIR_FMT, FILE_FMT,
                  THREAD_FMT, ND_INDEX, ND_HEADER, ND_MAP, ND_LEAF, REC_DIR, REC_FILE, REC_DTHREAD,
                  REC_FTHREAD, CNID_ROOT, CNID_ROOT_PARENT, CNID_EXTENTS, CNID_CATALOG, HFSError,
                  unpack_extents)


class Node:
    __slots__ = ("num", "flink", "blink", "type", "height", "recs", "offsets")


class BTree:
    """A B*-tree file held in memory."""

    def __init__(self, data, name="tree"):
        self.data, self.name = data, name
        if len(data) < NODE:
            raise HFSError("%s: file shorter than one node" % name)
        hdr = self.node(0, raw_size=True)
        if hdr.type != ND_HEADER or len(hdr.recs) < 3:
            raise HFSError("%s: node 0 is not a header node" % name)
        (self.depth, self.root, self.nrecs, self.first_leaf, self.last_leaf, self.node_size,
         self.key_len, self.nnodes, self.free) = struct.unpack_from(BTH_FMT, hdr.recs[0])
        if self.node_size != NODE:
            raise HFSError("%s: node size %d not supported (HFS uses 512)" % (name, self.node_size))
        self.header = hdr

    def node(self, num, raw_size=False):
        off = num * NODE
        if num < 0 or off + NODE > len(self.data):
            raise HFSError("%s: node %d is outside the file" % (self.name, num))
        raw = self.data[off:off + NODE]
        n = Node()
        n.num = num
        n.flink, n.blink, n.type, n.height, nrecs, _ = struct.unpack_from(">IIBBHH", raw)
        if 14 + 2 * (nrecs + 1) > NODE:
            raise HFSError("%s: node %d claims %d records" % (self.name, num, nrecs))
        offs = [struct.unpack_from(">H", raw, NODE - 2 * (i + 1))[0] for i in range(nrecs + 1)]
        limit = NODE - 2 * (nrecs + 1)
        prev = 14
        if nrecs and offs[0] != 14:
            raise HFSError("%s: node %d: first record not at offset 14" % (self.name, num))
        for o in offs:
            if o < prev or o > limit:
                raise HFSError("%s: node %d has a bad record offset table" % (self.name, num))
            prev = o
        n.offsets = offs
        n.recs = [raw[offs[i]:offs[i + 1]] for i in range(nrecs)]
        return n

    def map_bits(self):
        """Node allocation bitmap -> (bytearray of bits, list of map node numbers)."""
        bits = bytearray(self.header.recs[2])
        maps, nxt, seen = [], self.header.flink, set()
        while nxt:
            if nxt in seen:
                raise HFSError("%s: map node chain loops" % self.name)
            seen.add(nxt)
            m = self.node(nxt)
            if m.type != ND_MAP or len(m.recs) != 1:
                raise HFSError("%s: node %d in the map chain is not a map node" % (self.name, nxt))
            bits += m.recs[0]
            maps.append(nxt)
            nxt = m.flink
        return bits, maps

    def leaves(self):
        """Yield leaf nodes by following the fLink chain."""
        num, seen = self.first_leaf, set()
        while num:
            if num in seen:
                raise HFSError("%s: leaf chain loops at node %d" % (self.name, num))
            seen.add(num)
            n = self.node(num)
            if n.type != ND_LEAF:
                raise HFSError("%s: node %d in the leaf chain is not a leaf" % (self.name, num))
            yield n
            num = n.flink

    def records(self):
        for n in self.leaves():
            for r in n.recs:
                yield r


def split_key(rec, lenient=False):
    """Record -> (key bytes without the length byte, data bytes).

    `lenient` repairs catalog records whose key-length byte is 0 (seen in
    images written by buggy tools) by deriving it from the name length, so
    such volumes can still be listed and extracted; `verify` reports them.
    """
    if not rec:
        raise HFSError("empty record")
    klen = rec[0]
    if klen == 0 and lenient and len(rec) > 6:
        klen = 6 + rec[6]
    end = 1 + klen
    if end & 1:
        end += 1
    if 1 + klen > len(rec):
        raise HFSError("key longer than its record")
    return rec[1:1 + klen], rec[end:]


def parse_cat_key(key):
    """Catalog key body -> (parent CNID, name bytes)."""
    if len(key) < 6:
        raise HFSError("catalog key too short")
    parent = struct.unpack_from(">I", key, 1)[0]
    nlen = key[5]
    if 6 + nlen > len(key) or nlen > 31:
        raise HFSError("catalog key name length %d is invalid" % nlen)
    return parent, bytes(key[6:6 + nlen])


class Entry:
    """One catalog entry (file or folder)."""
    is_dir = False

    def __init__(self):
        self.name = b""
        self.parent = 0
        self.cnid = 0
        self.created = self.modified = self.backup = 0

    @property
    def uname(self):
        return macroman.decode(self.name)


class DirEntry(Entry):
    is_dir = True

    def __init__(self):
        Entry.__init__(self)
        self.valence = 0
        self.flags = 0
        self.dinfo = self.dxinfo = b"\0" * 16


class FileEntry(Entry):
    def __init__(self):
        Entry.__init__(self)
        self.fil_flags = 0
        self.ftype = self.creator = b"????"
        self.flags = 0
        self.location = (0, 0)
        self.folder = 0
        self.fxinfo = b"\0" * 16
        self.dlen = self.dphys = self.rlen = self.rphys = 0
        self.dext = self.rext = ()

    @property
    def locked(self):
        return bool(self.fil_flags & 0x01)


def parse_cat_record(key, data):
    """-> ('dir'|'file'|'thread', object).  Threads give (type, parent, name)."""
    parent, name = parse_cat_key(key)
    if not data:
        raise HFSError("catalog record without data")
    rtype = data[0]
    if rtype == REC_DIR:
        if len(data) < 70:
            raise HFSError("short directory record")
        d = DirEntry()
        (_, _, d.flags, d.valence, d.cnid, d.created, d.modified, d.backup,
         d.dinfo, d.dxinfo, _) = struct.unpack_from(DIR_FMT, data)
        d.parent, d.name = parent, name
        return "dir", d
    if rtype == REC_FILE:
        if len(data) < 102:
            raise HFSError("short file record")
        f = FileEntry()
        (_, _, f.fil_flags, _, f.ftype, f.creator, f.flags, lv, lh, f.folder, f.cnid,
         _, f.dlen, f.dphys, _, f.rlen, f.rphys, f.created, f.modified, f.backup,
         f.fxinfo, _, dext, rext, _) = struct.unpack_from(FILE_FMT, data)
        f.location = (lv, lh)
        f.dext, f.rext = unpack_extents(dext), unpack_extents(rext)
        f.parent, f.name = parent, name
        return "file", f
    if rtype in (REC_DTHREAD, REC_FTHREAD):
        if len(data) < 46:
            raise HFSError("short thread record")
        _, _, _, tparent, tname = struct.unpack_from(THREAD_FMT, data)
        if tname[0] > 31:
            raise HFSError("thread record name too long")
        return "thread", (rtype, parent, tparent, bytes(tname[1:1 + tname[0]]))
    raise HFSError("unknown catalog record type %d" % rtype)


class Volume:
    """An HFS volume inside file object `f`, starting at sector `base`."""

    def __init__(self, f, base=0, nsect=None, load=True):
        self.f, self.base = f, base
        if nsect is None:
            f.seek(0, 2)
            nsect = f.tell() // SECTOR - base
        self.nsect = nsect
        raw = self.read_sectors(2, 1)
        if raw[:2] != MDB_SIG:
            raise HFSError("no HFS master directory block (signature %r)" % raw[:2])
        self.mdb_raw = raw
        self.mdb = dict(zip(MDB_FIELDS, struct.unpack_from(MDB_FMT, raw)))
        m = self.mdb
        self.alblk, self.nmal, self.alst = m["drAlBlkSiz"], m["drNmAlBlks"], m["drAlBlSt"]
        if self.alblk == 0 or self.alblk % SECTOR:
            raise HFSError("allocation block size %d is not a multiple of 512" % self.alblk)
        self.per = self.alblk // SECTOR
        nlen = min(m["drVN"][0], 27)
        self.name = bytes(m["drVN"][1:1 + nlen])
        self.xt_ext = unpack_extents(m["drXTExtRec"])
        self.ct_ext = unpack_extents(m["drCTExtRec"])
        self.overflow = {}           # (fork_type, cnid) -> [(start_fabn, [extents])]
        self.dirs, self.files, self.children, self.threads = {}, {}, {}, {}
        self.extents_tree = self.catalog_tree = None
        if load:
            self.load()

    @property
    def uname(self):
        return macroman.decode(self.name)

    # -- raw access -----------------------------------------------------------
    def read_sectors(self, sector, count):
        self.f.seek((self.base + sector) * SECTOR)
        data = self.f.read(count * SECTOR)
        if len(data) != count * SECTOR:
            raise HFSError("read past the end of the image (sector %d)" % sector)
        return data

    def bitmap(self):
        nsec = -(-((self.nmal + 7) // 8) // SECTOR)
        return self.read_sectors(self.mdb["drVBMSt"], nsec)

    def read_extents(self, extents, length):
        out, left = [], length
        for start, count in extents:
            if not count or left <= 0:
                continue
            if start + count > self.nmal:
                raise HFSError("extent %d+%d is outside the volume" % (start, count))
            n = min(left, count * self.alblk)
            self.f.seek((self.base + self.alst + start * self.per) * SECTOR)
            data = self.f.read(n)
            if len(data) != n:
                raise HFSError("read past the end of the image")
            out.append(data)
            left -= n
        if left > 0:
            raise HFSError("extents are %d bytes short of the fork length" % left)
        return b"".join(out)

    # -- trees ----------------------------------------------------------------
    def all_extents(self, cnid, fork_type, first):
        """First three extents + any overflow extents, in file order."""
        ext = [e for e in first if e[1]]
        for _, more in sorted(self.overflow.get((fork_type, cnid), [])):
            ext += [e for e in more if e[1]]
        return ext

    def load(self):
        m = self.mdb
        self.extents_tree = BTree(self.read_extents(self.xt_ext, m["drXTFlSize"]), "extents tree")
        for rec in self.extents_tree.records():
            key, data = split_key(rec)
            if len(key) < 7 or len(data) < 12:
                raise HFSError("bad extents-overflow record")
            fork, cnid, fabn = struct.unpack_from(">BIH", key)
            self.overflow.setdefault((fork, cnid), []).append((fabn, unpack_extents(data[:12])))
        self.catalog_tree = BTree(
            self.read_extents(self.all_extents(CNID_CATALOG, 0, self.ct_ext), m["drCTFlSize"]),
            "catalog tree")
        for rec in self.catalog_tree.records():
            key, data = split_key(rec, lenient=True)
            kind, obj = parse_cat_record(key, data)
            if kind == "dir":
                self.dirs[obj.cnid] = obj
                self.children.setdefault(obj.parent, []).append(obj)
            elif kind == "file":
                self.files[obj.cnid] = obj
                self.children.setdefault(obj.parent, []).append(obj)
            else:
                self.threads[obj[1]] = obj

    # -- catalog access ---------------------------------------------------------
    @property
    def root(self):
        return self.dirs.get(CNID_ROOT)

    def listdir(self, d):
        """Children of DirEntry `d` in catalog (Mac sort) order."""
        return self.children.get(d.cnid, [])

    def lookup(self, path):
        """'bin/SHELL', ':bin:SHELL' or '' (root) -> entry, or None."""
        path = path.strip()
        parts = [p for p in (path.split(":") if ":" in path else path.replace("\\", "/").split("/")) if p]
        cur = self.root
        for i, part in enumerate(parts):
            if cur is None or not cur.is_dir:
                return None
            want = macroman.sort_key(macroman.encode(part))
            cur = next((c for c in self.listdir(cur) if macroman.sort_key(c.name) == want), None)
        return cur

    def walk(self, d=None, prefix=()):
        """Yield (path tuple of name bytes, entry) depth-first in catalog order."""
        d = d or self.root
        if d is None:
            return
        for c in self.listdir(d):
            yield prefix + (c.name,), c
            if c.is_dir:
                for item in self.walk(c, prefix + (c.name,)):
                    yield item

    def fork_extents(self, f, rsrc=False):
        return self.all_extents(f.cnid, 0xFF if rsrc else 0x00, f.rext if rsrc else f.dext)

    def read_fork(self, f, rsrc=False):
        length = f.rlen if rsrc else f.dlen
        if not length:
            return b""
        return self.read_extents(self.fork_extents(f, rsrc), length)

    def free_bytes(self):
        return self.mdb["drFreeBks"] * self.alblk

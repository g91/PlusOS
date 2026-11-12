"""HFS on-disk structures and a from-scratch formatter / writer.

Everything here follows Inside Macintosh: Files, chapter 2 ("Data
Organization on Volumes").  A volume of N 512-byte sectors looks like this:

    sector 0-1        boot blocks
    sector 2          Master Directory Block (MDB)
    sector 3 ...      volume bitmap (1 bit per allocation block, MSB first)
    sector drAlBlSt   allocation block 0 ... drNmAlBlks-1
                        extents-overflow B*-tree, catalog B*-tree, file forks
    sector N-2        alternate MDB
    sector N-1        unused

The builder writes a brand-new volume in one pass: all files get contiguous
extents (at most 3 pieces when the free space is split by a reserved raw
area), so the extents-overflow tree is always empty - but valid.
"""
import os
import struct

from . import macroman

SECTOR = 512
NODE = 512
MAC_EPOCH_DELTA = 2082844800        # seconds from 1904-01-01 to 1970-01-01

MDB_SIG = b"BD"
FIRST_USER_CNID = 16
CNID_ROOT_PARENT = 1
CNID_ROOT = 2
CNID_EXTENTS = 3
CNID_CATALOG = 4

ND_INDEX, ND_HEADER, ND_MAP, ND_LEAF = 0, 1, 2, 0xFF
REC_DIR, REC_FILE, REC_DTHREAD, REC_FTHREAD = 1, 2, 3, 4
CAT_KEYLEN = 37                     # bthKeyLen of the catalog tree
EXT_KEYLEN = 7                      # bthKeyLen of the extents tree
HDR_MAP_BITS = 256 * 8              # node-allocation bits in the header node
MAP_NODE_BITS = (NODE - 14 - 4) * 8  # ... and in each additional map node

MAX_SECTORS = (2 * 1024 * 1024 * 1024) // SECTOR - 1    # < 2 GiB volumes
MAX_FORK = 0x7FFFFFFF

MDB_FMT = ">2sIIHHHHHIIHIH28sIHIIIHII32sHHHI12sI12s"
MDB_FIELDS = ("drSigWord drCrDate drLsMod drAtrb drNmFls drVBMSt drAllocPtr drNmAlBlks "
              "drAlBlkSiz drClpSiz drAlBlSt drNxtCNID drFreeBks drVN drVolBkUp drVSeqNum "
              "drWrCnt drXTClpSiz drCTClpSiz drNmRtDirs drFilCnt drDirCnt drFndrInfo "
              "drVCSize drVBMCSize drCtlCSize drXTFlSize drXTExtRec drCTFlSize drCTExtRec").split()
MDB_SIZE = struct.calcsize(MDB_FMT)
assert MDB_SIZE == 162

BTH_FMT = ">HIIIIHHII"              # B-tree header record (+76 reserved bytes)
DIR_FMT = ">BBHHIIII16s16s16s"      # 70 bytes
FILE_FMT = ">BBBB4s4sHhhHIHIIHIIIII16sH12s12sI"   # 102 bytes
THREAD_FMT = ">BB8sI32s"            # 46 bytes
assert struct.calcsize(DIR_FMT) == 70 and struct.calcsize(FILE_FMT) == 102
assert struct.calcsize(THREAD_FMT) == 46


class HFSError(Exception):
    pass


# ---- small helpers ----------------------------------------------------------

def pstr(raw, size):
    """Pascal string in a fixed `size`-byte field."""
    if len(raw) > size - 1:
        raise HFSError("string %r too long for a %d-byte field" % (raw, size))
    return bytes([len(raw)]) + raw + b"\0" * (size - 1 - len(raw))


def pack_extents(extents):
    ext = list(extents) + [(0, 0)] * (3 - len(extents))
    if len(ext) != 3:
        raise HFSError("more than 3 extents")
    return struct.pack(">6H", *[v for e in ext for v in e])


def unpack_extents(raw):
    v = struct.unpack(">6H", raw)
    return [(v[0], v[1]), (v[2], v[3]), (v[4], v[5])]


def fourcc(text):
    """'TEXT' / b'TEXT' / '0x54455854' -> 4 bytes."""
    if isinstance(text, (bytes, bytearray)):
        raw = bytes(text)
    elif isinstance(text, str) and len(text) == 10 and text.lower().startswith("0x"):
        raw = bytes.fromhex(text[2:])
    else:
        raw = str(text).encode("mac_roman")
    if len(raw) > 4:
        raise HFSError("type/creator code %r is longer than 4 characters" % (text,))
    return raw.ljust(4, b" ")


def fourcc_str(raw):
    """4 bytes -> printable text if possible, else '0x........'."""
    if all(32 <= b < 127 for b in raw):
        return raw.decode("ascii")
    return "0x" + raw.hex()


def geometry(nsect, alblk=None):
    """Volume geometry for `nsect` sectors -> (alblk_bytes, vbm_sectors, nm_al_blks, al_bl_st).

    Same arithmetic as hfsutils' hformat (and what Apple's formatter produces):
    the smallest allocation block that keeps drNmAlBlks in 16 bits.
    """
    if nsect < 64:
        raise HFSError("volume of %d sectors is too small for HFS" % nsect)
    if nsect > MAX_SECTORS:
        raise HFSError("volume of %d sectors exceeds the 2 GB HFS limit of the Plus/SE-era File Manager"
                       % nsect)
    if alblk is None:
        lpa = 1 + ((nsect - 6) >> 16)
    else:
        if alblk % SECTOR or alblk <= 0:
            raise HFSError("allocation block size must be a multiple of 512")
        lpa = alblk // SECTOR
    while True:
        vbm = (nsect // lpa + 0x0FFF) >> 12
        nmal = (nsect - 5 - vbm) // lpa
        if nmal <= 0xFFFF:
            break
        if alblk is not None:
            raise HFSError("allocation block size %d is too small for %d sectors "
                           "(would need %d allocation blocks, max 65535)" % (alblk, nsect, nmal))
        lpa += 1
    if nmal < 16:
        raise HFSError("volume too small")
    return lpa * SECTOR, vbm, nmal, 3 + vbm


# ---- the tree fed to the builder --------------------------------------------

class Dir:
    """A folder to be written.  `name` is MacRoman bytes."""
    is_dir = True

    def __init__(self, name, created=0, modified=0, backup=0, dinfo=b"", dxinfo=b"", flags=0):
        self.name = name
        self.created, self.modified, self.backup = created, modified, backup
        self.dinfo = bytes(dinfo).ljust(16, b"\0")
        self.dxinfo = bytes(dxinfo).ljust(16, b"\0")
        self.flags = flags
        self.children = []
        self.cnid = 0

    def add(self, child):
        self.children.append(child)
        return child


class File:
    """A file to be written.  `data` / `rsrc` are bytes or a host path (str)."""
    is_dir = False

    def __init__(self, name, data=b"", rsrc=b"", ftype=b"????", creator=b"????", flags=0,
                 location=(0, 0), folder=0, fxinfo=b"", created=0, modified=0, backup=0,
                 locked=False):
        self.name = name
        self.data, self.rsrc = data, rsrc
        self.ftype, self.creator = fourcc(ftype), fourcc(creator)
        self.flags, self.location, self.folder = flags, location, folder
        self.fxinfo = bytes(fxinfo).ljust(16, b"\0")
        self.created, self.modified, self.backup = created, modified, backup
        self.locked = locked
        self.cnid = 0
        self.dext, self.rext = [], []
        self.dlen = self.rlen = 0


def _src_len(src):
    if src is None:
        return 0
    if isinstance(src, (bytes, bytearray)):
        return len(src)
    return os.path.getsize(src)


def _src_chunks(src, length):
    if isinstance(src, (bytes, bytearray)):
        if len(src) != length:
            raise HFSError("source changed size while building")
        yield bytes(src)
        return
    got = 0
    with open(src, "rb") as fh:
        while True:
            chunk = fh.read(1 << 20)
            if not chunk:
                break
            got += len(chunk)
            yield chunk
    if got != length:
        raise HFSError("%s changed size while the image was being built" % src)


# ---- B*-tree construction -----------------------------------------------------

def _pack_node(flink, blink, ntype, height, recs):
    node = bytearray(NODE)
    struct.pack_into(">IIBBHH", node, 0, flink, blink, ntype, height, len(recs), 0)
    off = 14
    for i, rec in enumerate(recs):
        struct.pack_into(">H", node, NODE - 2 * (i + 1), off)
        node[off:off + len(rec)] = rec
        off += len(rec)
    struct.pack_into(">H", node, NODE - 2 * (len(recs) + 1), off)
    if off > NODE - 2 * (len(recs) + 1):
        raise HFSError("internal error: node overflow")
    return node


def _split(sizes):
    """Greedy packing of record sizes into nodes -> list of record counts."""
    counts, used, n = [], 14, 0
    for size in sizes:
        if n and used + size + 2 * (n + 2) > NODE:
            counts.append(n)
            used, n = 14, 0
        if 14 + size + 4 > NODE:
            raise HFSError("record too large for a node")
        used += size
        n += 1
    if n:
        counts.append(n)
    return counts


def tree_nodes_needed(leaf_sizes, index_rec_size):
    """Number of leaf + index nodes a tree with these leaf record sizes needs."""
    level = _split(leaf_sizes)
    total = len(level)
    while len(level) > 1:
        level = _split([index_rec_size] * len(level))
        total += len(level)
    return total


def map_nodes_for(nnodes):
    if nnodes <= HDR_MAP_BITS:
        return 0
    return -(-(nnodes - HDR_MAP_BITS) // MAP_NODE_BITS)


def build_btree(records, keylen, nnodes):
    """Serialise a B*-tree file of `nnodes` nodes.

    `records` is a key-sorted list of (leaf_record_bytes, index_key_bytes);
    index_key_bytes is the fixed-length key (keylen+1 bytes) used in index
    nodes.  Returns (tree_bytes, info) with tree_bytes exactly nnodes*512 long.
    """
    nmap = map_nodes_for(nnodes)
    nodes = {}                               # node number -> bytes
    next_node = 1 + nmap
    depth = root = first_leaf = last_leaf = 0
    if records:
        level = [(rec, key) for rec, key in records]
        height = 1
        while True:
            counts = _split([len(r) for r, _ in level])
            numbers = list(range(next_node, next_node + len(counts)))
            next_node += len(counts)
            if next_node > nnodes:
                raise HFSError("B-tree file too small (%d nodes)" % nnodes)
            parent, pos = [], 0
            for i, cnt in enumerate(counts):
                recs = level[pos:pos + cnt]
                pos += cnt
                flink = numbers[i + 1] if i + 1 < len(numbers) else 0
                blink = numbers[i - 1] if i else 0
                ntype = ND_LEAF if height == 1 else ND_INDEX
                nodes[numbers[i]] = _pack_node(flink, blink, ntype, height, [r for r, _ in recs])
                parent.append((recs[0][1] + struct.pack(">I", numbers[i]), recs[0][1]))
            if height == 1:
                first_leaf, last_leaf = numbers[0], numbers[-1]
            if len(counts) == 1:
                root, depth = numbers[0], height
                break
            level = parent
            height += 1
    used = next_node                          # header + map nodes + tree nodes
    if used > nnodes:
        raise HFSError("B-tree file too small (%d nodes, need %d)" % (nnodes, used))

    bits = bytearray((HDR_MAP_BITS + nmap * MAP_NODE_BITS) // 8)
    for n in range(used):
        bits[n >> 3] |= 0x80 >> (n & 7)

    hdr = struct.pack(BTH_FMT, depth, root, len(records), first_leaf, last_leaf,
                      NODE, keylen, nnodes, nnodes - used) + b"\0" * 76
    nodes[0] = _pack_node(1 if nmap else 0, 0, ND_HEADER, 0,
                          [hdr, b"\0" * 128, bytes(bits[:HDR_MAP_BITS // 8])])
    for m in range(nmap):
        lo = HDR_MAP_BITS // 8 + m * (MAP_NODE_BITS // 8)
        nodes[1 + m] = _pack_node(2 + m if m + 1 < nmap else 0, m, ND_MAP, 0,
                                  [bytes(bits[lo:lo + MAP_NODE_BITS // 8])])
    out = bytearray(nnodes * NODE)
    for n, raw in nodes.items():
        out[n * NODE:(n + 1) * NODE] = raw
    return bytes(out), {"depth": depth, "root": root, "records": len(records),
                        "used_nodes": used, "nodes": nnodes, "map_nodes": nmap}


def cat_leaf_key(parent, name):
    """Variable-length catalog key as stored in leaf nodes (padded to even)."""
    key = struct.pack(">BBI", 6 + len(name), 0, parent) + bytes([len(name)]) + name
    return key + (b"\0" if len(key) & 1 else b"")


def cat_index_key(parent, name):
    """Fixed-length (38-byte) catalog key as stored in index nodes."""
    return struct.pack(">BBI", CAT_KEYLEN, 0, parent) + pstr(name, 32)


# ---- the volume builder -----------------------------------------------------

class VolumeBuilder:
    """Format and populate one HFS volume of `nsect` 512-byte sectors."""

    def __init__(self, nsect, name, alblk=None, created=0, modified=0, backup=0,
                 finder_info=b"", write_count=None, catalog_fraction=128):
        if not 1 <= len(name) <= macroman.MAX_VOLNAME:
            raise HFSError("volume name must be 1..27 MacRoman characters")
        if b":" in name:
            raise HFSError("volume name may not contain ':'")
        self.nsect, self.name = nsect, name
        self.alblk, self.vbm_sectors, self.nmal, self.alst = geometry(nsect, alblk)
        self.per = self.alblk // SECTOR
        self.created, self.modified, self.backup = created, modified, backup
        self.finder_info = bytes(finder_info).ljust(32, b"\0")
        self.write_count = write_count
        self.catalog_fraction = catalog_fraction
        self.used = bytearray(self.nmal)
        self.cursor = 0
        self.last_end = 0
        self.root = Dir(name, created, modified)
        self.blessed = None                  # a Dir: its CNID goes to drFndrInfo[0] (System Folder)
        self.reserved = []                   # (first_sector, last_sector, a0, a1)
        self.raw_writes = []                 # (sector, bytes)
        self.boot = b""

    # -- raw (non-filesystem) areas ------------------------------------------
    def reserve(self, first_sector, last_sector):
        """Mark the allocation blocks covering [first, last] as used.

        Identical arithmetic to tools/install.py Volume.reserve()."""
        a0 = (first_sector - self.alst) // self.per
        a1 = (last_sector - self.alst) // self.per
        if a0 < 0 or a1 >= self.nmal:
            raise HFSError(
                "reserved raw area (sectors %d-%d) lies outside the HFS volume (%d allocation "
                "blocks of %d bytes starting at sector %d): the volume must be at least %.1f MB"
                % (first_sector, last_sector, self.nmal, self.alblk, self.alst,
                   (last_sector + 8 + self.per) * SECTOR / 1048576.0))
        for a in range(a0, a1 + 1):
            self.used[a] = 1
        self.reserved.append((first_sector, last_sector, a0, a1))
        return a0, a1

    def raw_write(self, sector, data):
        self.raw_writes.append((sector, bytes(data)))

    # -- allocation -----------------------------------------------------------
    def _run_at(self, start):
        """Length of the free run starting at `start`."""
        end = self.used.find(1, start)
        return (self.nmal if end < 0 else end) - start

    def _take(self, start, count):
        self.used[start:start + count] = b"\1" * count
        self.last_end = max(self.last_end, start + count)

    def alloc(self, nblocks):
        """Allocate `nblocks` allocation blocks -> list of (start, count), <= 3 extents."""
        if nblocks == 0:
            return []
        if nblocks > 0xFFFF:
            raise HFSError("fork too large for this volume")
        pos = self.used.find(0, self.cursor)
        self.cursor = pos if pos >= 0 else self.nmal
        runs = []
        while 0 <= pos < self.nmal:
            length = self._run_at(pos)
            if length >= nblocks:
                self._take(pos, nblocks)
                return [(pos, nblocks)]
            runs.append((pos, length))
            pos = self.used.find(0, pos + length)
        # no single run is big enough: use up to three, largest first
        runs.sort(key=lambda r: -r[1])
        pick, need = [], nblocks
        for start, length in runs[:3]:
            take = min(length, need)
            pick.append((start, take))
            need -= take
            if not need:
                break
        if need:
            raise HFSError("volume full: cannot place a fork of %d allocation blocks (%d bytes each); "
                           "%d blocks free" % (nblocks, self.alblk, self.used.count(0)))
        pick.sort()
        for start, count in pick:
            self._take(start, count)
        return pick

    def _blocks(self, nbytes):
        return -(-nbytes // self.alblk)

    # -- layout ---------------------------------------------------------------
    def _walk(self):
        """Assign CNIDs depth-first in catalog order; return (dirs, files)."""
        dirs, files = [], []
        next_cnid = [FIRST_USER_CNID]

        def visit(d, path):
            seen = {}
            for c in d.children:
                if not 1 <= len(c.name) <= macroman.MAX_NAME or b":" in c.name:
                    raise HFSError("illegal HFS name %r in %s" % (c.name, path or ":"))
                k = macroman.sort_key(c.name)
                if k in seen:
                    raise HFSError("names %r and %r in folder %s are the same to HFS "
                                   "(case/diacritics are not significant)"
                                   % (macroman.decode(seen[k]), macroman.decode(c.name), path or ":"))
                seen[k] = c.name
            d.children.sort(key=lambda c: macroman.sort_key(c.name))
            for c in d.children:
                c.cnid = next_cnid[0]
                next_cnid[0] += 1
                c.parent = d.cnid
                if c.is_dir:
                    dirs.append(c)
                    visit(c, path + ":" + macroman.decode(c.name))
                else:
                    files.append(c)

        self.root.cnid = CNID_ROOT
        self.root.parent = CNID_ROOT_PARENT
        self.root.name = self.name
        visit(self.root, "")
        return dirs, files, next_cnid[0]

    def build(self, out, base=0):
        """Write the whole volume to file object `out` at byte offset `base`.

        The target region must already be zero-filled.  Returns a summary dict."""
        dirs, files, next_cnid = self._walk()
        alldirs = [self.root] + dirs
        if self.blessed is not None:
            self.finder_info = struct.pack(">I", self.blessed.cnid) + self.finder_info[4:]

        # 1. size the trees
        leaf_sizes = []
        for d in alldirs:
            leaf_sizes.append(len(cat_leaf_key(d.parent, d.name)) + 70)
            leaf_sizes.append(8 + 46)
        for f in files:
            leaf_sizes.append(len(cat_leaf_key(f.parent, f.name)) + 102)
        need = 1 + tree_nodes_needed(leaf_sizes, 38 + 4)
        clump_blocks = max(self.nmal // self.catalog_fraction, self._blocks(8 * NODE), 1)
        xt_blocks = clump_blocks
        ct_blocks = clump_blocks
        while True:
            ct_nodes = ct_blocks * self.alblk // NODE
            if need + max(need // 2, 8) + map_nodes_for(ct_nodes) <= ct_nodes:
                break
            ct_blocks += clump_blocks
        xt_nodes = xt_blocks * self.alblk // NODE

        # 2. allocate: extents tree, catalog tree, then every fork in catalog order
        try:
            xt_ext = self.alloc(xt_blocks)
            ct_ext = self.alloc(ct_blocks)
            for f in files:
                f.dlen, f.rlen = _src_len(f.data), _src_len(f.rsrc)
                if f.dlen > MAX_FORK or f.rlen > MAX_FORK:
                    raise HFSError("%r: forks are limited to 2 GB" % macroman.decode(f.name))
                f.dext = self.alloc(self._blocks(f.dlen))
                f.rext = self.alloc(self._blocks(f.rlen))
        except HFSError as e:
            total = sum(f.dlen + f.rlen for f in files)
            raise HFSError("%s (content: %d files, %d bytes; volume: %d bytes)"
                           % (e, len(files), total, self.nmal * self.alblk))

        # 3. catalog records
        recs = []
        for d in alldirs:
            body = struct.pack(DIR_FMT, REC_DIR, 0, d.flags, len(d.children), d.cnid,
                               d.created, d.modified, d.backup, d.dinfo, d.dxinfo, b"")
            recs.append((macroman.catalog_key(d.parent, d.name),
                         cat_leaf_key(d.parent, d.name) + body, cat_index_key(d.parent, d.name)))
            thread = struct.pack(THREAD_FMT, REC_DTHREAD, 0, b"", d.parent, pstr(d.name, 32))
            recs.append((macroman.catalog_key(d.cnid, b""),
                         cat_leaf_key(d.cnid, b"") + thread, cat_index_key(d.cnid, b"")))
        for f in files:
            body = struct.pack(
                FILE_FMT, REC_FILE, 0, 0x01 if f.locked else 0, 0, f.ftype, f.creator,
                f.flags & 0xFFFF, f.location[0], f.location[1], f.folder & 0xFFFF, f.cnid,
                0, f.dlen, sum(c for _, c in f.dext) * self.alblk,
                0, f.rlen, sum(c for _, c in f.rext) * self.alblk,
                f.created, f.modified, f.backup, f.fxinfo, 0,
                pack_extents(f.dext), pack_extents(f.rext), 0)
            recs.append((macroman.catalog_key(f.parent, f.name),
                         cat_leaf_key(f.parent, f.name) + body, cat_index_key(f.parent, f.name)))
        recs.sort(key=lambda r: r[0])
        ct_bytes, ct_info = build_btree([(r[1], r[2]) for r in recs], CAT_KEYLEN, ct_nodes)
        xt_bytes, xt_info = build_btree([], EXT_KEYLEN, xt_nodes)

        # 4. MDB + bitmap
        free = self.used.count(0)
        alloc_ptr = self.last_end if self.last_end < self.nmal else 0
        root_files = sum(1 for c in self.root.children if not c.is_dir)
        root_dirs = len(self.root.children) - root_files
        wrcnt = self.write_count if self.write_count is not None else len(files) + len(dirs) + 1
        mdb = struct.pack(
            MDB_FMT, MDB_SIG, self.created, self.modified, 0x0100, root_files, 3, alloc_ptr,
            self.nmal, self.alblk, 4 * self.alblk, self.alst, next_cnid, free,
            pstr(self.name, 28), self.backup, 0, wrcnt,
            xt_blocks * self.alblk, clump_blocks * self.alblk, root_dirs, len(files), len(dirs),
            self.finder_info, 0, 0, 0,
            xt_blocks * self.alblk, pack_extents(xt_ext),
            ct_blocks * self.alblk, pack_extents(ct_ext)).ljust(SECTOR, b"\0")
        bitmap = bytearray(self.vbm_sectors * SECTOR)
        for a in range(self.nmal):
            if self.used[a]:
                bitmap[a >> 3] |= 0x80 >> (a & 7)

        # 5. write
        def put(sector, data):
            out.seek(base + sector * SECTOR)
            out.write(data)

        def put_fork(extents, chunks):
            ext = iter(extents)
            room, pos = 0, 0
            for chunk in chunks:
                view = memoryview(chunk)
                while len(view):
                    if not room:
                        start, count = next(ext)
                        pos = base + (self.alst + start * self.per) * SECTOR
                        room = count * self.alblk
                    n = min(room, len(view))
                    out.seek(pos)
                    out.write(view[:n])
                    view = view[n:]
                    pos += n
                    room -= n

        if len(self.boot) > 2 * SECTOR:
            raise HFSError("boot blocks hold 1024 bytes, loader is %d" % len(self.boot))
        put(0, self.boot.ljust(2 * SECTOR, b"\0"))
        put(2, mdb)
        put(3, bitmap)
        put_fork(xt_ext, [xt_bytes])
        put_fork(ct_ext, [ct_bytes])
        for f in files:
            if f.dlen:
                put_fork(f.dext, _src_chunks(f.data, f.dlen))
            if f.rlen:
                put_fork(f.rext, _src_chunks(f.rsrc, f.rlen))
        for sector, data in self.raw_writes:
            if sector < 0 or sector * SECTOR + len(data) > (self.nsect - 2) * SECTOR:
                raise HFSError("raw write at sector %d is outside the volume" % sector)
            put(sector, data)
        put(self.nsect - 2, mdb)
        return {
            "sectors": self.nsect, "alloc_block_size": self.alblk, "alloc_blocks": self.nmal,
            "first_alloc_sector": self.alst, "bitmap_sectors": self.vbm_sectors,
            "free_blocks": free, "files": len(files), "dirs": len(dirs),
            "next_cnid": next_cnid, "catalog": ct_info, "extents": xt_info,
            "catalog_extents": ct_ext, "extents_extents": xt_ext,
        }

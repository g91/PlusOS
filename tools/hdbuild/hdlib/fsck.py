"""`hdbuild.py verify`: consistency check of a disk image (an fsck for HFS).

Checks the partition map, the MDB against its alternate copy, both B*-trees
(node map, links, key order, index keys, record counts), every catalog
record (threads, valences, counts, CNIDs), every extent (inside the volume,
marked in the bitmap, no overlaps), the free-block count, and the PlusOS raw
areas (kernel magic, app directory, reservation in the bitmap).
"""
import struct

from . import apm, macroman
from .hfs import (SECTOR, NODE, MDB_FMT, MDB_FIELDS, MDB_SIG, ND_INDEX, ND_HEADER, ND_MAP, ND_LEAF,
                  CNID_ROOT, CNID_ROOT_PARENT, CNID_EXTENTS, CNID_CATALOG, FIRST_USER_CNID,
                  CAT_KEYLEN, EXT_KEYLEN, HFSError, unpack_extents, geometry)
from .hfsread import Volume, BTree, split_key as _split_key, parse_cat_record, parse_cat_key
from . import image as _image


class Report:
    def __init__(self):
        self.errors, self.warnings, self.notes = [], [], []

    def error(self, msg):
        if len(self.errors) < 200:
            self.errors.append(msg)

    def warn(self, msg):
        if len(self.warnings) < 200:
            self.warnings.append(msg)

    def note(self, msg):
        self.notes.append(msg)

    @property
    def ok(self):
        return not self.errors


def _cat_sort_key(key):
    parent, name = parse_cat_key(key)
    return (parent, macroman.sort_key(name))


def _ext_sort_key(key):
    if len(key) < 7:
        raise HFSError("extents key too short")
    return struct.unpack_from(">BIH", key)


def check_tree(tree, sort_key, rep, fixed_index_keylen):
    """Structural check of one B*-tree.  Returns the list of leaf records."""
    lenient = fixed_index_keylen == CAT_KEYLEN

    def split_key(rec):
        return _split_key(rec, lenient=lenient)

    name = tree.name
    total = len(tree.data) // NODE
    if tree.nnodes != total:
        rep.error("%s: header says %d nodes, file holds %d" % (name, tree.nnodes, total))
    try:
        bits, maps = tree.map_bits()
    except HFSError as e:
        rep.error(str(e))
        return []
    if len(bits) * 8 < total:
        rep.error("%s: node map covers %d nodes, file has %d" % (name, len(bits) * 8, total))

    def used(n):
        return n < len(bits) * 8 and bool(bits[n >> 3] & (0x80 >> (n & 7)))

    reach = {0} | set(maps)
    leaves_in_order = []
    records = []

    def descend(num, height, lo_key, depth_left):
        """Visit node `num`; returns its first key (sort form) or None."""
        if num in reach:
            rep.error("%s: node %d is referenced more than once" % (name, num))
            return None
        if num <= 0 or num >= total:
            rep.error("%s: node pointer %d is outside the tree" % (name, num))
            return None
        reach.add(num)
        try:
            n = tree.node(num)
        except HFSError as e:
            rep.error(str(e))
            return None
        want = ND_LEAF if height == 1 else ND_INDEX
        if n.type != want:
            rep.error("%s: node %d has type %d, expected %d at height %d" % (name, num, n.type, want, height))
            return None
        if n.height != height:
            rep.error("%s: node %d has height %d, expected %d" % (name, num, n.height, height))
        if not n.recs:
            rep.error("%s: node %d is empty" % (name, num))
            return None
        keys = []
        for i, rec in enumerate(n.recs):
            try:
                if rec and rec[0] == 0:
                    rep.error("%s: node %d record %d has key length 0 (corrupt or deleted record)"
                              % (name, num, i))
                key, data = split_key(rec)
                keys.append(sort_key(key))
            except HFSError as e:
                rep.error("%s: node %d record %d: %s" % (name, num, i, e))
                return None
            if height > 1:
                if rec[0] != fixed_index_keylen:
                    rep.error("%s: index node %d record %d has key length %d (must be %d)"
                              % (name, num, i, rec[0], fixed_index_keylen))
                if len(data) < 4:
                    rep.error("%s: index node %d record %d has no node pointer" % (name, num, i))
                    return None
        for i in range(1, len(keys)):
            if not keys[i - 1] < keys[i]:
                rep.error("%s: node %d: keys %d and %d are out of order" % (name, num, i - 1, i))
        if lo_key is not None and keys[0] < lo_key:
            rep.error("%s: node %d starts below its index key" % (name, num))
        if height == 1:
            leaves_in_order.append(n)
            for rec in n.recs:
                records.append(rec)
        else:
            for i, rec in enumerate(n.recs):
                key, data = split_key(rec)
                child = struct.unpack_from(">I", data)[0]
                first = descend(child, height - 1, keys[i], depth_left - 1)
                if first is not None and i + 1 < len(keys) and not first < keys[i + 1]:
                    rep.error("%s: subtree of node %d record %d overlaps the next key" % (name, num, i))
        return keys[0]

    if tree.depth == 0:
        if tree.root or tree.nrecs or tree.first_leaf or tree.last_leaf:
            rep.error("%s: empty tree (depth 0) but root/records/leaf links are set" % name)
    else:
        if tree.depth > 8:
            rep.error("%s: depth %d is not plausible" % (name, tree.depth))
            return []
        descend(tree.root, tree.depth, None, tree.depth)
        # sibling links, level by level (leaf level against the header's links)
        if leaves_in_order:
            if tree.first_leaf != leaves_in_order[0].num:
                rep.error("%s: bthFNode is %d, first leaf is %d" % (name, tree.first_leaf, leaves_in_order[0].num))
            if tree.last_leaf != leaves_in_order[-1].num:
                rep.error("%s: bthLNode is %d, last leaf is %d" % (name, tree.last_leaf, leaves_in_order[-1].num))
            prev = 0
            for i, n in enumerate(leaves_in_order):
                nxt = leaves_in_order[i + 1].num if i + 1 < len(leaves_in_order) else 0
                if n.blink != prev or n.flink != nxt:
                    rep.error("%s: leaf %d has links b=%d f=%d, expected b=%d f=%d"
                              % (name, n.num, n.blink, n.flink, prev, nxt))
                prev = n.num
        _check_index_links(tree, rep)
        try:
            chain = [n.num for n in tree.leaves()]
            if chain != [n.num for n in leaves_in_order]:
                rep.error("%s: the leaf chain differs from the leaves reachable through the index" % name)
        except HFSError as e:
            rep.error(str(e))
        prev = None
        for rec in records:
            k = sort_key(split_key(rec)[0])
            if prev is not None and not prev < k:
                rep.error("%s: leaf records are not in strictly ascending key order" % name)
                break
            prev = k
    if tree.nrecs != len(records):
        rep.error("%s: header says %d records, leaves hold %d" % (name, tree.nrecs, len(records)))
    nused = 0
    for n in range(total):
        u = used(n)
        nused += u
        if n in reach and not u:
            rep.error("%s: node %d is in use but free in the node map" % (name, n))
        elif u and n not in reach:
            rep.error("%s: node %d is marked used in the node map but is not part of the tree" % (name, n))
    for n in range(total, len(bits) * 8):
        if used(n):
            rep.error("%s: node map marks node %d beyond the end of the file" % (name, n))
            break
    if tree.free != total - nused:
        rep.error("%s: header says %d free nodes, node map says %d" % (name, tree.free, total - nused))
    return records


def _check_index_links(tree, rep):
    """fLink/bLink of every index level, walking down the leftmost spine."""
    num, height = tree.root, tree.depth
    while height > 1:
        try:
            level, cur, prev, seen = [], num, 0, set()
            while cur:
                if cur in seen:
                    rep.error("%s: index level %d sibling chain loops" % (tree.name, height))
                    return
                seen.add(cur)
                n = tree.node(cur)
                if n.type != ND_INDEX or n.height != height:
                    rep.error("%s: node %d in the level-%d sibling chain is not an index node of that level"
                              % (tree.name, cur, height))
                    return
                if n.blink != prev:
                    rep.error("%s: index node %d has bLink %d, expected %d" % (tree.name, cur, n.blink, prev))
                level.append(n)
                prev, cur = cur, n.flink
            first = tree.node(num)
            _, data = _split_key(first.recs[0], lenient=True)
            num = struct.unpack_from(">I", data)[0]
        except (HFSError, IndexError, struct.error) as e:
            rep.error("%s: %s" % (tree.name, e))
            return
        height -= 1


def check_volume(f, base, nsect, rep, kernel_block=None, apps_block=None, expect_raw=None):
    """Check the HFS volume at sector `base`.  Returns the Volume or None."""
    try:
        vol = Volume(f, base, nsect, load=False)
    except HFSError as e:
        rep.error(str(e))
        return None
    m = vol.mdb
    # ---- MDB ----
    try:
        alt = vol.read_sectors(nsect - 2, 1)
        if alt[:2] != MDB_SIG:
            rep.error("alternate MDB (sector %d) is missing" % (nsect - 2))
        elif alt[:162] != vol.mdb_raw[:162]:
            am = dict(zip(MDB_FIELDS, struct.unpack_from(MDB_FMT, alt)))
            diff = [k for k in MDB_FIELDS if am[k] != m[k]]
            hard = [k for k in diff if k in ("drNmAlBlks", "drAlBlkSiz", "drAlBlSt", "drVBMSt", "drCrDate")]
            (rep.error if hard else rep.warn)("alternate MDB differs from the MDB in: %s" % ", ".join(diff))
    except HFSError as e:
        rep.error("alternate MDB: %s" % e)
    if m["drVBMSt"] != 3:
        rep.warn("drVBMSt is %d (normally 3)" % m["drVBMSt"])
    bm_sectors = -(-((vol.nmal + 7) // 8) // SECTOR)
    if m["drVBMSt"] + bm_sectors > vol.alst:
        rep.error("volume bitmap (%d sectors at %d) overlaps the allocation area at sector %d"
                  % (bm_sectors, m["drVBMSt"], vol.alst))
    if vol.alst + vol.nmal * vol.per > nsect - 2:
        rep.error("allocation area (%d blocks of %d bytes from sector %d) runs into the alternate MDB "
                  "or past the volume end (%d sectors)" % (vol.nmal, vol.alblk, vol.alst, nsect))
    if not 1 <= m["drVN"][0] <= 27:
        rep.error("volume name length %d is invalid" % m["drVN"][0])
    if m["drClpSiz"] % vol.alblk or not m["drClpSiz"]:
        rep.warn("drClpSiz %d is not a multiple of the allocation block size" % m["drClpSiz"])
    if not m["drAtrb"] & 0x0100:
        rep.warn("volume was not unmounted cleanly (drAtrb bit 8 clear)")
    for label, size, ext in (("extents", m["drXTFlSize"], vol.xt_ext), ("catalog", m["drCTFlSize"], vol.ct_ext)):
        blocks = sum(c for _, c in ext)
        if size % NODE:
            rep.error("%s file size %d is not a multiple of the node size" % (label, size))
        if size > blocks * vol.alblk and label == "extents":
            rep.error("%s file size %d exceeds its extents (%d blocks)" % (label, size, blocks))

    # ---- trees ----
    try:
        vol.load()
    except (HFSError, struct.error) as e:
        rep.error("cannot load the volume: %s" % e)
        return None
    check_tree(vol.extents_tree, _ext_sort_key, rep, EXT_KEYLEN)
    recs = check_tree(vol.catalog_tree, _cat_sort_key, rep, CAT_KEYLEN)
    if vol.extents_tree.key_len != EXT_KEYLEN:
        rep.error("extents tree: bthKeyLen is %d, must be %d" % (vol.extents_tree.key_len, EXT_KEYLEN))
    if vol.catalog_tree.key_len != CAT_KEYLEN:
        rep.error("catalog tree: bthKeyLen is %d, must be %d" % (vol.catalog_tree.key_len, CAT_KEYLEN))

    # ---- catalog records ----
    dirs, files, threads = {}, {}, {}
    children = {}
    for rec in recs:
        try:
            key, data = _split_key(rec, lenient=True)
            kind, obj = parse_cat_record(key, data)
        except HFSError as e:
            rep.error("catalog: %s" % e)
            continue
        if kind == "thread":
            rtype, cnid, tparent, tname = obj
            if parse_cat_key(key)[1]:
                rep.error("catalog: thread record of CNID %d has a non-empty key name" % cnid)
            threads[cnid] = obj
            continue
        if not obj.name:
            rep.error("catalog: CNID %d has an empty name" % obj.cnid)
        if b":" in obj.name:
            rep.error("catalog: name %r contains ':'" % obj.uname)
        table = dirs if kind == "dir" else files
        if obj.cnid in dirs or obj.cnid in files:
            rep.error("catalog: CNID %d is used twice" % obj.cnid)
        table[obj.cnid] = obj
        children.setdefault(obj.parent, []).append(obj)
    root = dirs.get(CNID_ROOT)
    if root is None:
        rep.error("catalog: no root directory record (CNID 2)")
    else:
        if root.parent != CNID_ROOT_PARENT:
            rep.error("catalog: root directory has parent %d" % root.parent)
        if root.name != vol.name:
            rep.error("catalog: root directory is named %r but the volume is %r" % (root.uname, vol.uname))
    for cnid, d in dirs.items():
        t = threads.get(cnid)
        if t is None:
            rep.error("catalog: folder %r (CNID %d) has no thread record" % (d.uname, cnid))
        elif t[0] != 3 or t[2] != d.parent or t[3] != d.name:
            rep.error("catalog: thread record of folder %r (CNID %d) does not match it" % (d.uname, cnid))
        n = len(children.get(cnid, []))
        if d.valence != n:
            rep.error("catalog: folder %r (CNID %d) has valence %d but %d entries" % (d.uname, cnid, d.valence, n))
        if cnid != CNID_ROOT and d.parent not in dirs:
            rep.error("catalog: folder %r (CNID %d) is in missing folder %d" % (d.uname, cnid, d.parent))
    for cnid, t in threads.items():
        if t[0] == 3 and cnid not in dirs:
            rep.error("catalog: folder thread for CNID %d has no folder record" % cnid)
        if t[0] == 4:
            fobj = files.get(cnid)
            if fobj is None or fobj.parent != t[2] or fobj.name != t[3]:
                rep.error("catalog: file thread for CNID %d does not match a file record" % cnid)
    # no cycles: every folder must reach the root
    for cnid, d in dirs.items():
        seen, cur = set(), d
        while cur is not None and cur.cnid != CNID_ROOT:
            if cur.cnid in seen:
                rep.error("catalog: folder %r (CNID %d) is in a parent loop" % (d.uname, cnid))
                break
            seen.add(cur.cnid)
            cur = dirs.get(cur.parent)
    for cnid, fobj in files.items():
        if fobj.parent not in dirs:
            rep.error("catalog: file %r (CNID %d) is in missing folder %d" % (fobj.uname, cnid, fobj.parent))
    for cnid in list(dirs) + list(files):
        if cnid != CNID_ROOT and cnid < FIRST_USER_CNID:
            rep.error("catalog: CNID %d is in the reserved range" % cnid)
        if cnid >= m["drNxtCNID"]:
            rep.error("catalog: CNID %d is not below drNxtCNID (%d)" % (cnid, m["drNxtCNID"]))
    nroot = children.get(CNID_ROOT, [])
    counts = (("drNmFls", sum(1 for c in nroot if not c.is_dir)), ("drNmRtDirs", sum(1 for c in nroot if c.is_dir)),
              ("drFilCnt", len(files)), ("drDirCnt", len(dirs) - (1 if root is not None else 0)))
    for field, actual in counts:
        if m[field] != actual:
            rep.error("MDB %s is %d, catalog has %d" % (field, m[field], actual))

    # ---- extents, bitmap ----
    owner = [None] * vol.nmal

    def claim(extents, who, length, phys):
        blocks = 0
        for start, count in extents:
            if not count:
                continue
            if start + count > vol.nmal:
                rep.error("%s: extent %d+%d lies outside the volume (%d blocks)" % (who, start, count, vol.nmal))
                continue
            for a in range(start, start + count):
                if owner[a] is not None:
                    rep.error("%s: allocation block %d is also used by %s" % (who, a, owner[a]))
                    break
            for a in range(start, start + count):
                owner[a] = who
            blocks += count
        if phys is not None and blocks * vol.alblk != phys:
            # more blocks than the physical length = a file that was never closed (benign)
            (rep.error if blocks * vol.alblk < phys else rep.warn)(
                "%s: physical length %d does not match its extents (%d blocks = %d bytes)"
                % (who, phys, blocks, blocks * vol.alblk))
        if length > blocks * vol.alblk:
            rep.error("%s: logical length %d exceeds its allocation (%d bytes)" % (who, length, blocks * vol.alblk))

    claim(vol.xt_ext, "extents tree", m["drXTFlSize"], m["drXTFlSize"])
    claim(vol.all_extents(CNID_CATALOG, 0, vol.ct_ext), "catalog tree", m["drCTFlSize"], m["drCTFlSize"])
    for cnid, fobj in files.items():
        who = "file %r (CNID %d)" % (fobj.uname, cnid)
        claim(vol.all_extents(cnid, 0x00, fobj.dext), who + " data fork", fobj.dlen, fobj.dphys)
        claim(vol.all_extents(cnid, 0xFF, fobj.rext), who + " resource fork", fobj.rlen, fobj.rphys)
    for (fork, cnid), lst in vol.overflow.items():
        if cnid not in files and cnid != CNID_CATALOG:
            rep.error("extents tree: overflow extents for unknown CNID %d" % cnid)
    try:
        bm = vol.bitmap()
    except HFSError as e:
        rep.error("volume bitmap: %s" % e)
        return vol
    own = bytearray(1 if o is not None else 0 for o in owner)
    free = 0
    unmarked = 0
    for a in range(vol.nmal):
        bit = bm[a >> 3] & (0x80 >> (a & 7))
        if not bit:
            free += 1
            if own[a]:
                unmarked += 1
                if unmarked <= 5:
                    rep.error("allocation block %d belongs to %s but is free in the volume bitmap" % (a, owner[a]))
    if unmarked > 5:
        rep.error("... %d allocation blocks in use but free in the bitmap in total" % unmarked)
    if free != m["drFreeBks"]:
        rep.error("MDB drFreeBks is %d, the bitmap has %d free blocks" % (m["drFreeBks"], free))
    if m["drAllocPtr"] >= vol.nmal and vol.nmal:
        rep.warn("drAllocPtr %d is beyond the last allocation block" % m["drAllocPtr"])

    # ---- boot blocks + PlusOS raw areas ----
    boot = vol.read_sectors(0, 2)
    if boot[:2] == b"LK":
        rep.note("boot blocks: 'LK' signature, version $%04X" % struct.unpack_from(">H", boot, 6)[0])
    elif boot.strip(b"\0"):
        rep.warn("boot blocks are not empty but lack the 'LK' signature")
    else:
        rep.note("boot blocks: empty (volume is not bootable)")
    raw = _image.detect_raw(vol, own, kernel_block, apps_block)
    runs = _image.unowned_runs(vol, own)
    raw_run = None
    if raw:
        k = raw["kernel"]
        s0, s1 = raw["run"]
        if not k["reserved"]:
            rep.error("raw kernel at block %d is NOT reserved in the volume bitmap (the Mac may overwrite it)"
                      % k["block"])
        else:
            raw_run = ((s0 - vol.alst) // vol.per, (s1 - vol.alst) // vol.per)
            last_k = k["block"] + -(-k["length"] // SECTOR) - 1
            if last_k > s1:
                rep.error("raw kernel (blocks %d-%d) extends past its reserved area (ends at block %d)"
                          % (k["block"], last_k, s1))
        rep.note("raw kernel: %d bytes at block %d" % (k["length"], k["block"]))
        if boot[:2] == b"LK" and struct.pack(">I", k["block"] * SECTOR) not in boot:
            rep.warn("the boot loader does not contain the kernel byte offset %d (block %d)"
                     % (k["block"] * SECTOR, k["block"]))
        if "apps" in raw:
            a = raw["apps"]
            if a["count"] > (a["dir_blocks"] * SECTOR - 8) // 24:
                rep.error("raw app directory claims %d entries" % a["count"])
            end = a["block"] + a["dir_blocks"]
            for name, start, length in a["entries"]:
                nb = -(-length // SECTOR)
                if start < end or start + nb > a["end"]:
                    rep.error("raw app %r (blocks %d-%d) overlaps or lies outside the reserved app area "
                              "(blocks %d-%d)" % (name, start, start + nb - 1, a["block"], a["end"] - 1))
                end = max(end, start + nb)
            reserved = a["end"] - a["block"]
            if 0 <= reserved - _image.manifest.APPS_RESERVE < vol.per:
                reserved = _image.manifest.APPS_RESERVE      # allocation-block rounding
            rep.note("raw app store: %d apps at block %d, %d of %d reserved blocks used"
                     % (len(a["entries"]), a["block"], end - a["block"], reserved))
    elif expect_raw:
        rep.error("no PlusOS raw kernel found (no 'PLOS' image in a reserved area)")
    else:
        rep.note("no PlusOS raw kernel area found")
    for a0, a1 in runs:
        if raw_run and raw_run[0] <= a0 and a1 <= raw_run[1]:
            continue
        rep.warn("allocation blocks %d-%d are marked used but belong to no file "
                 "(lost blocks or a foreign reserved area)" % (a0, a1))
    return vol


def check_partition_map(img, rep):
    info = img.apm
    size_blocks = img.size // SECTOR
    if info["block_size"] != SECTOR:
        rep.error("DDR block size is %d (expected 512)" % info["block_size"])
    if info["block_count"] > size_blocks:
        rep.error("DDR says %d blocks but the image holds only %d" % (info["block_count"], size_blocks))
    elif info["block_count"] < size_blocks - 1:
        rep.warn("DDR says %d blocks, the image holds %d" % (info["block_count"], size_blocks))
    parts = info["partitions"]
    if not parts:
        rep.error("no partition map entries")
        return
    if len(parts) != parts[0]["map_entries"]:
        rep.error("partition map says %d entries, found %d" % (parts[0]["map_entries"], len(parts)))
    spans = []
    for p in parts:
        if p["map_entries"] != parts[0]["map_entries"]:
            rep.error("partition %d: pmMapBlkCnt differs from entry 1" % p["index"])
        if p["start"] + p["blocks"] > size_blocks:
            rep.error("partition %d (%s) extends past the end of the image" % (p["index"], p["type"]))
        spans.append((p["start"], p["start"] + p["blocks"], p["index"]))
        if p["type"].startswith("Apple_Driver") and p["boot_size"]:
            img.f.seek(p["start"] * SECTOR)
            data = img.f.read(p["boot_size"])
            ck = apm.driver_checksum(data)
            if ck != p["boot_cksum"]:
                rep.error("driver partition %d: checksum $%04X, partition map says $%04X"
                          % (p["index"], ck, p["boot_cksum"]))
            if not any(d[0] == p["start"] for d in info["drivers"]):
                rep.error("driver partition %d is not listed in the DDR driver table" % p["index"])
    spans.sort()
    for i in range(1, len(spans)):
        if spans[i][0] < spans[i - 1][1]:
            rep.error("partitions %d and %d overlap" % (spans[i - 1][2], spans[i][2]))
    mp = next((p for p in parts if p["type"] == "Apple_partition_map"), None)
    if mp is None:
        rep.error("no Apple_partition_map entry")
    elif mp["start"] != 1 or mp["blocks"] < len(parts):
        rep.error("Apple_partition_map entry does not cover the map")
    for d in info["drivers"]:
        if not any(p["start"] <= d[0] < p["start"] + p["blocks"] and p["type"].startswith("Apple_Driver")
                   for p in parts):
            rep.error("DDR driver entry at block %d is not inside a driver partition" % d[0])
    if not apm.find_driver(info):
        rep.note("no Apple_Driver partition: will not boot from real SCSI hardware / BlueSCSI "
                 "(fine for emulators with their own disk driver)")


def verify(path, kernel_block=None, apps_block=None, expect_raw=None):
    """Verify image `path` -> Report."""
    rep = Report()
    try:
        img = _image.Opened(path)
    except (_image.BuildError, OSError) as e:
        rep.error(str(e))
        return rep
    with img:
        if img.apm:
            check_partition_map(img, rep)
            rep.note("Apple_HFS partition at block %d, %d blocks" % (img.base, img.nsect))
        else:
            rep.note("bare HFS volume, %d blocks" % img.nsect)
        try:
            vol = check_volume(img.f, img.base, img.nsect, rep, kernel_block, apps_block, expect_raw)
        except (HFSError, struct.error, IndexError) as e:
            rep.error("volume check aborted: %s" % e)
            vol = None
        if vol is not None and vol.catalog_tree is not None:
            rep.note("volume '%s': %d files, %d folders, %d of %d allocation blocks free, catalog depth %d"
                     % (vol.uname, len(vol.files), max(len(vol.dirs) - 1, 0), vol.mdb["drFreeBks"],
                        vol.nmal, vol.catalog_tree.depth))
    return rep

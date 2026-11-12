"""folder -> disk image (build_image) and disk image -> folder (extract)."""
import os
import struct
import time

from . import apm, forks, macroman, manifest
from .hfs import SECTOR, HFSError, VolumeBuilder, Dir, File, fourcc, fourcc_str, MDB_SIG
from .hfsread import Volume
from .manifest import ManifestError, MANIFEST_NAME, RSRC_DIR, BOOT_DIR, RESERVED_TOP

DIR_MAGIC = b"PLDR"
KERNEL_MAGIC = b"PLOS"            # at offset 4 of a PlusOS .pex image
PRESET_SIZES = {"floppy1440": 1474560, "floppy800": 819200}


class BuildError(Exception):
    pass


def _log_default(msg):
    print(msg)


# ---- opening images ---------------------------------------------------------

class Opened:
    """An image file plus where its HFS volume lives."""

    def __init__(self, path, mode="rb"):
        self.path = path
        self.f = open(path, mode)
        try:
            self._probe(path)
        except Exception:
            self.f.close()
            raise

    def _probe(self, path):
        self.f.seek(0, 2)
        self.size = self.f.tell()
        self.apm = apm.parse(self.f)
        self.hfs_part = None
        if self.apm:
            self.hfs_part = apm.find_hfs(self.apm)
            if not self.hfs_part:
                raise BuildError("%s: partition map has no Apple_HFS partition" % path)
            self.base, self.nsect = self.hfs_part["start"], self.hfs_part["blocks"]
            self.format = "hda"
        else:
            self.f.seek(2 * SECTOR)
            if self.f.read(2) != MDB_SIG:
                raise BuildError("%s: neither a partitioned disk (no 'ER' block 0) nor a bare "
                                 "HFS volume (no 'BD' at block 2)" % path)
            self.base, self.nsect = 0, self.size // SECTOR
            self.format = "dsk"
        if (self.base + self.nsect) * SECTOR > self.size:
            raise BuildError("%s: HFS partition (blocks %d..%d) extends past the end of the file"
                             % (path, self.base, self.base + self.nsect - 1))

    def volume(self, load=True):
        return Volume(self.f, self.base, self.nsect, load=load)

    def close(self):
        self.f.close()

    def __enter__(self):
        return self

    def __exit__(self, *a):
        self.close()


# ---- raw areas (kernel + app store) -----------------------------------------

def owned_blocks(vol):
    """bytearray(nmal): 1 where an allocation block belongs to a tree or a fork."""
    own = bytearray(vol.nmal)

    def mark(extents):
        for start, count in extents:
            if count and start + count <= vol.nmal:
                own[start:start + count] = b"\1" * count

    mark(vol.xt_ext)
    mark(vol.all_extents(4, 0, vol.ct_ext))
    for f in vol.files.values():
        mark(vol.fork_extents(f, False))
        mark(vol.fork_extents(f, True))
    return own


def unowned_runs(vol, own=None):
    """Allocated-in-the-bitmap but owned by no file: [(a0, a1)] (inclusive)."""
    own = own if own is not None else owned_blocks(vol)
    bm = vol.bitmap()
    runs, start = [], None
    for a in range(vol.nmal):
        hit = bool(bm[a >> 3] & (0x80 >> (a & 7))) and not own[a]
        if hit and start is None:
            start = a
        elif not hit and start is not None:
            runs.append((start, a - 1))
            start = None
    if start is not None:
        runs.append((start, vol.nmal - 1))
    return runs


def detect_raw(vol, own=None, kernel_block=None, apps_block=None):
    """Find the PlusOS raw kernel / app store of a volume.

    Returns {} or {'run': (first_sector, last_sector), 'kernel': {...}, 'apps': {...}}.
    """
    own = own if own is not None else owned_blocks(vol)
    runs = unowned_runs(vol, own)
    limit = vol.nsect - 2

    def sector(n):
        return vol.read_sectors(n, 1) if 0 <= n < limit else b""

    found = {}
    cands = []
    for a0, a1 in runs:
        s0 = vol.alst + a0 * vol.per
        s1 = vol.alst + (a1 + 1) * vol.per - 1
        for s in range(s0, min(s0 + vol.per, s1 + 1)):
            cands.append((s, s0, s1))
    first = []
    for kb in (kernel_block, manifest.KERNEL_BLOCK, manifest.FLOPPY_KERNEL_BLOCK):
        if kb is None or not vol.alst <= kb < limit:
            continue
        ab = (kb - vol.alst) // vol.per
        if ab >= vol.nmal or own[ab]:
            continue                      # belongs to a file: cannot be the raw kernel
        run = next(((s0, s1) for a0, a1 in runs
                    for s0, s1 in [(vol.alst + a0 * vol.per, vol.alst + (a1 + 1) * vol.per - 1)]
                    if s0 <= kb <= s1), (kb, None))
        first.append((kb, run[0], run[1]))
    cands = first + cands
    for s, s0, s1 in cands:
        if sector(s)[4:8] == KERNEL_MAGIC:
            found["kernel"] = {"block": s}
            found["run"] = (s0, s1)
            break
    if "kernel" not in found:
        return {}
    kb = found["kernel"]["block"]
    s0, s1 = found["run"]
    reserved = s1 is not None
    if s1 is None:
        s1 = min(kb + 8000, limit) - 1
    # app store directory: 'PLDR' at the start of a sector after the kernel
    ab = None
    order = [apps_block] if apps_block is not None else []
    order += [kb + (manifest.APPS_BLOCK - manifest.KERNEL_BLOCK)]
    order += [s for s in range(kb + 1, s1 + 1)]
    for s in order:
        if kb < s <= s1 and sector(s)[:4] == DIR_MAGIC:
            ab = s
            break
    area_end = (ab if ab is not None else s1 + 1)
    head = sector(kb)
    klen = struct.unpack_from(">I", head, 12)[0]
    if not 64 <= klen <= (area_end - kb) * SECTOR:
        raw = vol.read_sectors(kb, area_end - kb).rstrip(b"\0")
        klen = len(raw)
    found["kernel"].update(length=klen, reserved=reserved)
    if ab is not None:
        dirblk = vol.read_sectors(ab, manifest.APPS_DIR_BLOCKS)
        count = struct.unpack_from(">I", dirblk, 4)[0]
        entries = []
        for i in range(min(count, (len(dirblk) - 8) // 24)):
            name, start, length = struct.unpack_from(">16sII", dirblk, 8 + i * 24)
            entries.append((name.split(b"\0")[0].decode("mac_roman"), start, length))
        found["apps"] = {"block": ab, "entries": entries, "count": count,
                         "end": s1 + 1, "dir_blocks": manifest.APPS_DIR_BLOCKS}
    return found


# ---- scanning the staged folder ---------------------------------------------

def _dates(entry, st):
    host = manifest.mac_from_unix(st.st_mtime)
    md = manifest.mac_from_iso(entry.get("modified"))
    cr = manifest.mac_from_iso(entry.get("created"))
    md = host if md is None else md
    cr = md if cr is None else cr
    return cr, md, manifest.mac_from_iso(entry.get("backup")) or 0


def scan_tree(root, man, strict=False, warn=None):
    """Staged folder -> (list of top-level hfs.Dir/File, {posix relpath lower: File})."""
    warn = warn or (lambda m: None)
    index = {k.lower().strip("/"): v for k, v in (man.get("entries") or {}).items()}
    ignore = man.get("ignore") or []
    rules = man.get("rules") or manifest.DEFAULT_RULES
    by_path = {}
    used = set()

    def walk(hostdir, rel):
        out = []
        try:
            names = sorted(os.listdir(hostdir))
        except OSError as e:
            raise BuildError("cannot read %s: %s" % (hostdir, e))
        doubles = {n[2:]: n for n in names if n.startswith("._")}
        for name in names:
            if not rel and name in RESERVED_TOP:
                continue
            if name.startswith("._"):
                if name[2:] not in names:
                    warn("AppleDouble file %s has no companion file; ignored" % os.path.join(hostdir, name))
                continue
            if manifest.ignored(ignore, name):
                continue
            path = os.path.join(hostdir, name)
            relp = (rel + "/" + name) if rel else name
            entry = index.get(relp.lower(), {})
            used.add(relp.lower())
            try:
                if "mac_name" in entry:
                    mname = macroman.encode(entry["mac_name"], strict=True)
                else:
                    mname = macroman.from_host(name, strict=strict, warn=warn)
            except macroman.NameError_ as e:
                raise BuildError("%s: %s" % (path, e))
            st = os.stat(path)
            cr, md, bk = _dates(entry, st)
            if os.path.isdir(path):
                dinfo = bytearray(16)
                struct.pack_into(">H", dinfo, 8, manifest.parse_flags(entry.get("flags")))
                loc = entry.get("location") or (0, 0)
                struct.pack_into(">hh", dinfo, 10, int(loc[0]), int(loc[1]))
                d = Dir(mname, cr, md, bk, dinfo=bytes(dinfo))
                d.children = walk(path, relp)
                out.append(d)
                continue
            if not os.path.isfile(path):
                warn("skipping %s (not a regular file)" % path)
                continue
            ftype = creator = None
            flags, fxinfo, rsrc = 0, b"", b""
            loc = (0, 0)
            if name in doubles:
                with open(os.path.join(hostdir, doubles[name]), "rb") as fh:
                    ad = forks.decode_applefile(fh.read())
                if ad is None:
                    warn("%s is not a valid AppleDouble file; ignored" % os.path.join(hostdir, doubles[name]))
                else:
                    ftype, creator, rsrc = ad.ftype, ad.creator, ad.rsrc
                    flags = ad.flags or 0
                    fxinfo = ad.fxinfo or b""
            side = os.path.join(root, RSRC_DIR, *relp.split("/")) + ".rsrc"
            if entry.get("rsrc"):
                side = os.path.join(root, *str(entry["rsrc"]).replace("\\", "/").split("/"))
                if not os.path.isfile(side):
                    raise BuildError("%s: resource fork %s (from the manifest) does not exist" % (relp, side))
                rsrc = side
            elif os.path.isfile(side):
                rsrc = side
            rule = manifest.match_rule(rules, relp)
            try:
                ftype = fourcc(entry.get("type") or ftype or rule.get("type", "????"))
                creator = fourcc(entry.get("creator") or creator or rule.get("creator", "????"))
            except (HFSError, UnicodeEncodeError) as e:
                raise BuildError("%s: bad type/creator: %s" % (relp, e))
            if "flags" in entry:
                flags = manifest.parse_flags(entry["flags"])
            if entry.get("location"):
                loc = (int(entry["location"][0]), int(entry["location"][1]))
            if entry.get("fxinfo"):
                fxinfo = bytes.fromhex(entry["fxinfo"])
            f = File(mname, data=path, rsrc=rsrc, ftype=ftype, creator=creator, flags=flags,
                     location=loc, fxinfo=fxinfo, created=cr, modified=md, backup=bk,
                     locked=bool(entry.get("locked")))
            by_path[relp.lower()] = f
            out.append(f)
        return out

    top = walk(root, "")
    for k in index:
        if k not in used:
            warn("manifest entry %r matches no file in %s" % (k, root))
    return top, by_path


# ---- building ---------------------------------------------------------------

def _resolve(root, path):
    if path is None:
        return None
    path = str(path)
    if os.path.isabs(path):
        return path
    return os.path.normpath(os.path.join(root, *path.replace("\\", "/").split("/")))


def _read(path, what):
    try:
        with open(path, "rb") as fh:
            return fh.read()
    except OSError as e:
        raise BuildError("cannot read %s %s: %s" % (what, path, e))


def load_driver(path, meta=None):
    """Read a driver blob (+ optional FILE.json metadata written by extract-driver)."""
    import json
    info = dict(meta or {})
    side = path + ".json"
    if os.path.exists(side) and not meta:
        with open(side, "r", encoding="utf-8") as fh:
            info.update(json.load(fh))
    info.pop("path", None)
    info["data"] = _read(path, "driver")
    if not info["data"]:
        raise BuildError("driver file %s is empty" % path)
    if isinstance(info.get("status"), str):
        info["status"] = int(info["status"], 0)
    return info


def plan_disk(man, opts, root, log):
    """Decide the disk layout -> dict(prefix, suffix, total, hfs_start, hfs_blocks, format, note)."""
    img = man["image"]
    fmt = opts.get("format") or img.get("format") or "hda"
    size_given = opts.get("size")
    size = manifest.parse_size(size_given or img.get("size") or "64M")
    template = opts.get("from_template")
    if template:
        with Opened(template) as t:
            if not t.apm:
                raise BuildError("--from-template %s: not a partitioned disk image" % template)
            hp = t.hfs_part
            t.f.seek(0)
            prefix = t.f.read(hp["start"] * SECTOR)
            total = t.size // SECTOR
            end = hp["start"] + hp["blocks"]
            last = all(p["start"] + p["blocks"] <= end or p["type"] == "Apple_Free"
                       for p in t.apm["partitions"])
            suffix = b""
            hfs_blocks = hp["blocks"]
            if size_given and size // SECTOR != total:
                if not last:
                    raise BuildError("--size with --from-template needs the Apple_HFS partition to be "
                                     "the last one in the template")
                total = size // SECTOR
                hfs_blocks = total - hp["start"]
                prefix = apm.retarget(prefix, hp["index"], total, hp["start"], hfs_blocks)
                log("  template resized: disk %d blocks, Apple_HFS %d blocks" % (total, hfs_blocks))
            elif end * SECTOR < t.size:
                t.f.seek(end * SECTOR)
                suffix = t.f.read()
            drv = apm.find_driver(t.apm)
            note = "partition map%s copied from %s" % (
                " and %s driver (%d bytes)" % (drv["type"], drv["boot_size"]) if drv else " (NO driver)",
                template)
            return {"prefix": prefix, "suffix": suffix, "total": total, "hfs_start": hp["start"],
                    "hfs_blocks": hfs_blocks, "format": "hda", "note": note, "driver": bool(drv)}
    if fmt == "dsk":
        return {"prefix": b"", "suffix": b"", "total": size // SECTOR, "hfs_start": 0,
                "hfs_blocks": size // SECTOR, "format": "dsk", "note": "bare HFS volume", "driver": False}
    if fmt != "hda":
        raise BuildError("unknown image format %r (use hda or dsk)" % fmt)
    driver = None
    if opts.get("driver"):
        driver = load_driver(opts["driver"])
        if opts.get("driver_type"):
            driver["type"] = opts["driver_type"]
        if opts.get("driver_bootargs"):
            driver["bootargs"] = opts["driver_bootargs"]
    elif img.get("driver") and not opts.get("no_driver"):
        meta = dict(img["driver"])
        dpath = _resolve(root, meta.get("path"))
        driver = load_driver(dpath, meta)
    total = size // SECTOR
    try:
        prefix, hfs_start, hfs_blocks = apm.build(
            total, driver, hfs_name=img.get("hfs_partition_name") or "MacOS",
            hfs_start=int(opts.get("hfs_start") or img.get("hfs_start") or apm.HFS_START))
    except apm.APMError as e:
        raise BuildError(str(e))
    note = ("with %s driver partition (%d bytes)" % (driver.get("type", "Apple_Driver43"), len(driver["data"]))
            if driver else "NO driver partition")
    return {"prefix": prefix, "suffix": b"", "total": total, "hfs_start": hfs_start,
            "hfs_blocks": hfs_blocks, "format": "hda", "note": note, "driver": bool(driver),
            "driver_info": driver}


def build_image(root, out, opts=None, log=_log_default, warn=None):
    """Build disk image `out` from staged folder `root`.  Returns a summary dict."""
    opts = dict(opts or {})
    warnings = []

    def _warn(msg):
        warnings.append(msg)
        (warn or (lambda m: log("  warning: " + m)))(msg)

    if not os.path.isdir(root):
        raise BuildError("staging folder %s does not exist - run `hdbuild.py stage` first" % root)
    man = manifest.load(root)
    if man.get("_missing"):
        _warn("%s has no %s: building a plain data volume (no boot blocks, no kernel)"
              % (root, MANIFEST_NAME))
    disk = plan_disk(man, opts, root, log)
    vol_cfg = man["volume"]
    try:
        vname = macroman.encode(opts.get("volume_name") or vol_cfg.get("name") or "PlusOS",
                                strict=bool(opts.get("strict_names")), limit=macroman.MAX_VOLNAME,
                                warn=_warn)
    except macroman.NameError_ as e:
        raise BuildError("volume name: %s" % e)

    now = opts.get("date")
    if now is None and os.environ.get("SOURCE_DATE_EPOCH"):
        now = manifest.mac_from_unix(int(os.environ["SOURCE_DATE_EPOCH"]))
    if now is None:
        now = manifest.mac_from_unix(time.time())
    created = manifest.mac_from_iso(vol_cfg.get("created"))
    modified = manifest.mac_from_iso(vol_cfg.get("modified"))
    created = now if created is None else created
    modified = max(now, created) if modified is None else modified
    alblk = opts.get("alloc_block_size") or vol_cfg.get("alloc_block_size")

    try:
        vb = VolumeBuilder(disk["hfs_blocks"], vname, alblk=alblk, created=created, modified=modified,
                           backup=manifest.mac_from_iso(vol_cfg.get("backup")) or 0,
                           finder_info=bytes.fromhex(vol_cfg.get("finder_info") or ""))
    except HFSError as e:
        raise BuildError(str(e))

    # ---- boot blocks ----
    loader_path = opts.get("loader") or _resolve(root, (man.get("boot") or {}).get("loader"))
    if loader_path:
        loader = _read(loader_path, "boot loader")
        if len(loader) > 1024:
            raise BuildError("loader is %d bytes, boot blocks hold 1024" % len(loader))
        if loader[:2] != b"LK":
            _warn("loader %s does not start with the 'LK' boot block signature" % loader_path)
        vb.boot = loader
    else:
        loader = b""

    # ---- raw kernel + raw app store (same layout as tools/install.py) ----
    raw = man.get("raw") or {}
    kcfg = raw.get("kernel")
    acfg = raw.get("apps") if not opts.get("no_raw_apps") else None
    summary_raw = {}
    top, by_path = scan_tree(root, man, strict=bool(opts.get("strict_names")), warn=_warn)
    if opts.get("kernel") and not kcfg:
        kcfg = {"block": manifest.KERNEL_BLOCK, "max_blocks": manifest.KERNEL_BLOCKS}
    if kcfg:
        kblock = int(opts.get("kernel_block") if opts.get("kernel_block") is not None else kcfg.get("block", manifest.KERNEL_BLOCK))
        kblocks = int(opts.get("kernel_blocks") or kcfg.get("max_blocks", manifest.KERNEL_BLOCKS))
        kpath = opts.get("kernel") or _resolve(root, kcfg.get("path"))
        kernel = _read(kpath, "kernel")
        if kernel[4:8] != KERNEL_MAGIC:
            raise BuildError("kernel %s has no PLOS header" % kpath)
        if len(kernel) > kblocks * SECTOR:
            raise BuildError("kernel is %d bytes, max %d (the loader reads KERNEL_BLOCKS=%d blocks; "
                             "raise KERNEL_BLOCKS / --kernel-blocks AND rebuild the loader to match)"
                             % (len(kernel), kblocks * SECTOR, kblocks))
        last = kblock + kblocks - 1
        ablock = None
        if acfg:
            ablock = int(opts.get("apps_block") if opts.get("apps_block") is not None else acfg.get("block", manifest.APPS_BLOCK))
            reserve = int(acfg.get("reserve_blocks", manifest.APPS_RESERVE))
            dirblks = int(acfg.get("dir_blocks", manifest.APPS_DIR_BLOCKS))
            if kblock + kblocks > ablock:
                raise BuildError("kernel area (blocks %d-%d) overlaps the app area at block %d"
                                 % (kblock, last, ablock))
            last = ablock + reserve - 1
        try:
            a0, a1 = vb.reserve(kblock, last)
        except HFSError as e:
            raise BuildError("%s\n  (the PlusOS raw kernel/app area occupies partition blocks %d-%d; "
                             "use a bigger --size, or a floppy preset for small media)" % (e, kblock, last))
        vb.raw_write(kblock, kernel)
        summary_raw["kernel"] = {"block": kblock, "bytes": len(kernel), "max_blocks": kblocks}
        summary_raw["reserved"] = (kblock, last, a1 - a0 + 1)
        if loader and struct.pack(">I", kblock * SECTOR) not in loader:
            _warn("the boot loader does not contain the kernel byte offset %d (block %d): it was "
                  "probably assembled for a different KERNEL_BLOCK and will not find the kernel"
                  % (kblock * SECTOR, kblock))
        if acfg:
            entries = acfg.get("entries") or []
            dirmax = (dirblks * SECTOR - 8) // 24
            if len(entries) > dirmax:
                raise BuildError("raw app store holds %d programs, have %d (trim raw.apps.entries in "
                                 "the manifest or pass --no-raw-apps)" % (dirmax, len(entries)))
            dirblk = bytearray(dirblks * SECTOR)
            dirblk[0:4] = DIR_MAGIC
            nxt = ablock + dirblks
            names = []
            for i, ent in enumerate(entries):
                if isinstance(ent, str):
                    ent = {"path": ent}
                epath = str(ent["path"]).replace("\\", "/")
                f = by_path.get(epath.lower().strip("/"))
                src = f.data if f is not None else _resolve(root, epath)
                data = _read(src, "raw app")
                name = str(ent.get("name") or os.path.basename(epath)).upper()
                nblocks = (len(data) + SECTOR - 1) // SECTOR
                if nxt + nblocks > ablock + reserve:
                    raise BuildError("raw app area full (%d blocks at %d); trim raw.apps.entries or "
                                     "pass --no-raw-apps" % (reserve, ablock))
                vb.raw_write(nxt, data)
                struct.pack_into(">16sII", dirblk, 8 + i * 24, name.encode("mac_roman")[:15], nxt, len(data))
                nxt += nblocks
                names.append(name)
            struct.pack_into(">I", dirblk, 4, len(entries))
            vb.raw_write(ablock, bytes(dirblk))
            summary_raw["apps"] = {"block": ablock, "count": len(entries), "names": names,
                                   "used_blocks": nxt - ablock, "reserve_blocks": reserve}

    vb.root.children = top
    blessed = vol_cfg.get("blessed")
    if blessed:
        cur, items = None, top
        for part in str(blessed).replace("\\", "/").strip("/").split("/"):
            want = macroman.sort_key(macroman.from_host(part))
            cur = next((c for c in items if c.is_dir and macroman.sort_key(c.name) == want), None)
            if cur is None:
                raise BuildError("volume.blessed: folder %r not found in %s" % (blessed, root))
            items = cur.children
        vb.blessed = cur

    # ---- write ----
    out = os.path.abspath(out)
    os.makedirs(os.path.dirname(out), exist_ok=True)
    tmp = out + ".tmp"
    try:
        with open(tmp, "wb") as f:
            f.truncate(disk["total"] * SECTOR)
            f.seek(0)
            f.write(disk["prefix"])
            if disk["suffix"]:
                end = (disk["hfs_start"] + disk["hfs_blocks"]) * SECTOR
                f.seek(end)
                f.write(disk["suffix"][:disk["total"] * SECTOR - end])
            info = vb.build(f, base=disk["hfs_start"] * SECTOR)
        try:
            os.replace(tmp, out)
        except PermissionError:
            raise BuildError("%s is locked - close the emulator (Mini vMac) first" % out)
    except HFSError as e:
        raise BuildError(str(e))
    finally:
        if os.path.exists(tmp):
            try:
                os.unlink(tmp)
            except OSError:
                pass

    info.update({"out": out, "format": disk["format"], "total_blocks": disk["total"],
                 "hfs_start": disk["hfs_start"], "hfs_blocks": disk["hfs_blocks"],
                 "volume": macroman.decode(vname), "raw": summary_raw, "note": disk["note"],
                 "has_driver": disk["driver"], "loader_bytes": len(loader), "warnings": warnings})
    return info


def describe_build(info, log=_log_default):
    log("%s" % info["out"])
    if info["format"] == "hda":
        log("  disk: %d blocks (%.1f MB), Apple partition map, %s"
            % (info["total_blocks"], info["total_blocks"] * SECTOR / 1048576.0, info["note"]))
        log("  Apple_HFS partition at block %d (%d blocks)" % (info["hfs_start"], info["hfs_blocks"]))
    else:
        log("  bare HFS volume: %d blocks (%d K)" % (info["total_blocks"], info["total_blocks"] // 2))
    log("  volume '%s': %d files, %d folders, alloc block %d bytes, %d of %d blocks free (%.1f MB)"
        % (info["volume"], info["files"], info["dirs"], info["alloc_block_size"], info["free_blocks"],
           info["alloc_blocks"], info["free_blocks"] * info["alloc_block_size"] / 1048576.0))
    c = info["catalog"]
    log("  catalog B-tree: %d records, depth %d, %d of %d nodes used"
        % (c["records"], c["depth"], c["used_nodes"], c["nodes"]))
    log("  boot blocks: loader %d bytes" % info["loader_bytes"] if info["loader_bytes"]
        else "  boot blocks: EMPTY (volume is not bootable)")
    raw = info["raw"]
    if "kernel" in raw:
        k = raw["kernel"]
        log("  kernel: %d bytes at block %d (max %d)" % (k["bytes"], k["block"], k["max_blocks"] * SECTOR))
        r = raw["reserved"]
        log("  reserved blocks %d-%d in the volume bitmap (%d alloc blocks)" % r)
    if "apps" in raw:
        a = raw["apps"]
        log("  raw app store: %d apps at block %d (%d of %d blocks used)"
            % (a["count"], a["block"], a["used_blocks"], a["reserve_blocks"]))
    if info["format"] == "hda" and not info["has_driver"]:
        log("  NOTE: no Apple_Driver partition - this image boots in emulators that supply their own "
            "disk driver,\n        but NOT on real hardware / BlueSCSI (see --driver and --from-template).")


# ---- extraction -------------------------------------------------------------

def _size_text(nbytes):
    for unit, mul in (("G", 1024 ** 3), ("M", 1024 ** 2), ("K", 1024)):
        if nbytes % mul == 0:
            return "%d%s" % (nbytes // mul, unit)
    return nbytes


def extract(image, dest, force=False, log=_log_default, kernel_block=None, apps_block=None):
    """Inverse of build_image: recreate the staged folder + manifest from an image."""
    if os.path.exists(dest) and os.listdir(dest) and not force:
        raise BuildError("%s exists and is not empty (use --force)" % dest)
    os.makedirs(dest, exist_ok=True)
    man = manifest.default_manifest()
    man["entries"] = {}
    with Opened(image) as img:
        vol = img.volume()
        times = []
        nfiles = ndirs = 0
        hostpath = {(): ""}
        for path, e in vol.walk():
            parent = hostpath[path[:-1]]
            hname = macroman.to_host(e.name, reserved=RESERVED_TOP if len(path) == 1 else ())
            rel = (parent + "/" + hname) if parent else hname
            hostpath[path] = rel
            target = os.path.join(dest, *rel.split("/"))
            ent = {}
            if e.is_dir:
                os.makedirs(target, exist_ok=True)
                ndirs += 1
                flags = struct.unpack_from(">H", e.dinfo, 8)[0]
                loc = struct.unpack_from(">hh", e.dinfo, 10)
                if flags:
                    ent["flags"] = "0x%04X" % flags
                if loc != (0, 0):
                    ent["location"] = list(loc)
            else:
                with open(target, "wb") as fh:
                    fh.write(vol.read_fork(e, False))
                nfiles += 1
                ent["type"], ent["creator"] = fourcc_str(e.ftype), fourcc_str(e.creator)
                if e.flags:
                    ent["flags"] = "0x%04X" % e.flags
                if e.location != (0, 0):
                    ent["location"] = list(e.location)
                if e.fxinfo.strip(b"\0"):
                    ent["fxinfo"] = e.fxinfo.hex()
                if e.locked:
                    ent["locked"] = True
                if e.rlen:
                    side = os.path.join(dest, RSRC_DIR, *rel.split("/")) + ".rsrc"
                    os.makedirs(os.path.dirname(side), exist_ok=True)
                    with open(side, "wb") as fh:
                        fh.write(vol.read_fork(e, True))
                    ent["rsrc"] = RSRC_DIR + "/" + rel + ".rsrc"
            ent["created"] = manifest.iso_from_mac(e.created)
            ent["modified"] = manifest.iso_from_mac(e.modified)
            if e.backup:
                ent["backup"] = manifest.iso_from_mac(e.backup)
            if macroman.decode(e.name) != hname:
                pass        # %XX escapes in the host name carry the difference
            man["entries"][rel] = ent
            times.append((target, e.modified))
        for target, md in reversed(times):
            try:
                ts = manifest.unix_from_mac(md)
                os.utime(target, (ts, ts))
            except (OSError, OverflowError, ValueError):
                pass

        m = vol.mdb
        man["volume"] = {"name": vol.uname, "created": manifest.iso_from_mac(m["drCrDate"]),
                         "modified": manifest.iso_from_mac(m["drLsMod"]),
                         "alloc_block_size": vol.alblk}
        if m["drVolBkUp"]:
            man["volume"]["backup"] = manifest.iso_from_mac(m["drVolBkUp"])
        fndr = m["drFndrInfo"]
        bless = struct.unpack_from(">I", fndr)[0]
        if bless:
            bpath = next((p for p, e in vol.walk() if e.is_dir and e.cnid == bless), None)
            if bpath is not None:                 # CNIDs change on rebuild: keep it as a path
                man["volume"]["blessed"] = hostpath[bpath]
                fndr = bytes(4) + fndr[4:]
        if fndr.strip(bytes(1)):
            man["volume"]["finder_info"] = fndr.hex()
        man["image"]["format"] = img.format
        man["image"]["size"] = _size_text(img.size)
        bootdir = os.path.join(dest, BOOT_DIR)
        os.makedirs(bootdir, exist_ok=True)
        if img.apm:
            man["image"]["hfs_start"] = img.base
            man["image"]["hfs_partition_name"] = img.hfs_part["name"]
            drv = apm.find_driver(img.apm)
            if drv:
                img.f.seek(drv["start"] * SECTOR)
                blob = img.f.read(drv["blocks"] * SECTOR)
                keep = max(drv["boot_size"], len(blob.rstrip(b"\0")))
                with open(os.path.join(bootdir, "driver.bin"), "wb") as fh:
                    fh.write(blob[:keep])
                dd = next((d for d in img.apm["drivers"] if d[0] == drv["start"]), None)
                man["image"]["driver"] = {
                    "path": BOOT_DIR + "/driver.bin", "type": drv["type"], "name": drv["name"],
                    "processor": drv["processor"], "bootargs": drv["bootargs"],
                    "boot_size": drv["boot_size"], "blocks": drv["blocks"],
                    "status": drv["status"], "dd_type": dd[2] if dd else 1}
        else:
            man["image"].pop("hfs_start", None)
            man["image"].pop("hfs_partition_name", None)
            man["image"]["driver"] = None

        boot = vol.read_sectors(0, 2)
        if boot.strip(b"\0"):
            with open(os.path.join(bootdir, "loader.bin"), "wb") as fh:
                fh.write(boot)
            man["boot"] = {"loader": BOOT_DIR + "/loader.bin"}
        else:
            man["boot"] = {"loader": None}

        raw = detect_raw(vol, kernel_block=kernel_block, apps_block=apps_block)
        man["raw"] = {"kernel": None, "apps": None}
        if raw:
            k = raw["kernel"]
            with open(os.path.join(bootdir, "kernel.pex"), "wb") as fh:
                fh.write(vol.read_sectors(k["block"], -(-k["length"] // SECTOR))[:k["length"]])
            s0, s1 = raw["run"]
            run_end = (s1 + 1) if k["reserved"] else None
            a1 = ((s1 - vol.alst) // vol.per) if k["reserved"] else None

            def fits(last):          # would reserving up to `last` end on the same alloc block?
                return a1 is not None and (last - vol.alst) // vol.per == a1

            need = -(-k["length"] // SECTOR)
            if "apps" in raw:
                a = raw["apps"]
                gap = a["block"] - k["block"]
                kmax = manifest.KERNEL_BLOCKS if need <= manifest.KERNEL_BLOCKS <= gap else gap
                if run_end is None or fits(a["block"] + manifest.APPS_RESERVE - 1):
                    reserve = manifest.APPS_RESERVE
                else:
                    reserve = run_end - a["block"]
                entries = []
                for name, start, length in a["entries"]:
                    data = vol.read_sectors(start, -(-length // SECTOR))[:length] if length else b""
                    f = vol.lookup("bin/" + name)
                    if f is not None and not f.is_dir and f.dlen == length and vol.read_fork(f) == data:
                        entries.append({"name": name, "path": hostpath[(vol.lookup("bin").name, f.name)]})
                    else:
                        adir = os.path.join(bootdir, "apps")
                        os.makedirs(adir, exist_ok=True)
                        hn = macroman.to_host(name.encode("mac_roman"))
                        with open(os.path.join(adir, hn), "wb") as fh:
                            fh.write(data)
                        entries.append({"name": name, "path": BOOT_DIR + "/apps/" + hn})
                man["raw"]["apps"] = {"block": a["block"], "reserve_blocks": reserve,
                                      "dir_blocks": a["dir_blocks"], "entries": entries}
            else:
                kmax = None
                for cand in (manifest.FLOPPY_KERNEL_BLOCKS, manifest.KERNEL_BLOCKS):
                    if need <= cand and fits(k["block"] + cand - 1):
                        kmax = cand
                        break
                if kmax is None:
                    kmax = (run_end - k["block"]) if run_end is not None else max(need, manifest.KERNEL_BLOCKS)
            man["raw"]["kernel"] = {"path": BOOT_DIR + "/kernel.pex", "block": k["block"], "max_blocks": kmax}
            if not k["reserved"]:
                log("  warning: the raw kernel at block %d is NOT reserved in the volume bitmap of this image"
                    % k["block"])
        if not os.listdir(bootdir):
            os.rmdir(bootdir)
    manifest.save(dest, man)
    log("extracted %d files, %d folders from '%s' to %s" % (nfiles, ndirs, man["volume"]["name"], dest))
    return man

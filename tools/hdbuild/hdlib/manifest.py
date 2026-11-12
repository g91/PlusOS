"""The staging manifest: <root>/.hdbuild.json

A host folder cannot hold Mac metadata (type/creator, Finder flags, resource
forks, Mac dates), nor the things that live outside the file system (boot
blocks, raw kernel area, raw app store, partition layout).  The manifest
holds all of that.  It is plain JSON, meant to be edited by hand; every key
is optional and files that are not listed get defaults from "rules".

See tools/hdbuild/README.md for the full schema.
"""
import copy
import datetime
import fnmatch
import json
import os
import re

from .hfs import MAC_EPOCH_DELTA, fourcc

MANIFEST_NAME = ".hdbuild.json"
RSRC_DIR = ".rsrc"                 # resource-fork sidecars: .rsrc/<path>.rsrc
BOOT_DIR = ".boot"                 # loader, kernel, driver (not part of the volume)
RESERVED_TOP = (MANIFEST_NAME, RSRC_DIR, BOOT_DIR)
VERSION = 1

# Defaults shared with config.mk / tools/install.py
KERNEL_BLOCK = 60000
KERNEL_BLOCKS = 512
APPS_BLOCK = 61000
APPS_RESERVE = 3000                # install.py RESERVE_END_OFFSET
APPS_DIR_BLOCKS = 4
# Floppy layout: Makefile `floppy:` builds the loader with KERNEL_OFFSET=51200
# (block 100) and KERNEL_BYTES=184320 (360 blocks).
FLOPPY_KERNEL_BLOCK = 100
FLOPPY_KERNEL_BLOCKS = 360

DEFAULT_RULES = [
    {"match": "*.pex", "type": "PLOS", "creator": "PLOS"},
    {"match": "*.cfg", "type": "TEXT", "creator": "ttxt"},
    {"match": "*.txt", "type": "TEXT", "creator": "ttxt"},
    {"match": "*.text", "type": "TEXT", "creator": "ttxt"},
    {"match": "*.ini", "type": "TEXT", "creator": "ttxt"},
    {"match": "*.log", "type": "TEXT", "creator": "ttxt"},
    {"match": "*.md", "type": "TEXT", "creator": "ttxt"},
    {"match": "*.c", "type": "TEXT", "creator": "ttxt"},
    {"match": "*.h", "type": "TEXT", "creator": "ttxt"},
    {"match": "*.bas", "type": "TEXT", "creator": "ttxt"},
    {"match": "*.sit", "type": "SIT!", "creator": "SIT!"},
    {"match": "*.pict", "type": "PICT", "creator": "ttxt"},
    {"match": "bin/*", "type": "PLOS", "creator": "PLOS"},
    {"match": "Games/*", "type": "PLOS", "creator": "PLOS"},
    {"match": "*", "type": "????", "creator": "????"},
]
DEFAULT_IGNORE = [".DS_Store", "Thumbs.db", "desktop.ini", "*~", ".git", ".gitignore", "__pycache__"]


class ManifestError(Exception):
    pass


def default_manifest():
    return {
        "hdbuild": VERSION,
        "machine": "se",
        "volume": {
            "name": "PlusOS",
            "created": None,             # "YYYY-MM-DDTHH:MM:SS" (Mac local time) or null = now
            "modified": None,
            "alloc_block_size": None,    # null = smallest that fits (512 * n)
            "blessed": None,             # path of a System Folder to bless (drFndrInfo[0]); null = none
        },
        "image": {
            "format": "hda",             # "hda" = DDR + partition map, "dsk" = bare HFS volume
            "size": "64M",
            "hfs_start": 96,             # first block of the Apple_HFS partition (hda)
            "hfs_partition_name": "MacOS",
            "driver": None,              # {"path": ".boot/driver.bin", "type": "Apple_Driver43", ...}
        },
        "boot": {"loader": BOOT_DIR + "/loader.bin"},
        "raw": {
            "kernel": {"path": BOOT_DIR + "/kernel.pex", "block": KERNEL_BLOCK, "max_blocks": KERNEL_BLOCKS},
            "apps": {"block": APPS_BLOCK, "reserve_blocks": APPS_RESERVE,
                     "dir_blocks": APPS_DIR_BLOCKS, "entries": []},
        },
        "rules": copy.deepcopy(DEFAULT_RULES),
        "ignore": list(DEFAULT_IGNORE),
        "entries": {},
    }


def _merge(base, over):
    for k, v in over.items():
        if isinstance(v, dict) and isinstance(base.get(k), dict) and k != "entries":
            _merge(base[k], v)
        else:
            base[k] = v
    return base


def load(root, required=False):
    """Load <root>/.hdbuild.json merged over the defaults."""
    path = os.path.join(root, MANIFEST_NAME)
    man = default_manifest()
    if not os.path.exists(path):
        if required:
            raise ManifestError("%s not found - run `hdbuild.py stage` first" % path)
        man["boot"]["loader"] = None
        man["raw"]["kernel"] = None
        man["raw"]["apps"] = None
        man["_missing"] = True
        return man
    try:
        with open(path, "r", encoding="utf-8") as fh:
            data = json.load(fh)
    except ValueError as e:
        raise ManifestError("%s: %s" % (path, e))
    if not isinstance(data, dict):
        raise ManifestError("%s: top level must be a JSON object" % path)
    if data.get("hdbuild", VERSION) > VERSION:
        raise ManifestError("%s was written by a newer hdbuild (schema %s)" % (path, data.get("hdbuild")))
    # "raw": {"kernel": null} / {"apps": null} must survive the merge as null
    _merge(man, data)
    return man


def save(root, man):
    out = {k: v for k, v in man.items() if not k.startswith("_")}
    path = os.path.join(root, MANIFEST_NAME)
    with open(path, "w", encoding="utf-8", newline="\n") as fh:
        json.dump(out, fh, indent=2, ensure_ascii=False)
        fh.write("\n")
    return path


# ---- values -----------------------------------------------------------------

_SIZE = re.compile(r"^\s*(\d+(?:\.\d+)?)\s*([kKmMgG]?)[iI]?[bB]?\s*$")


def parse_size(text):
    """'64M', '1440K', '800k', '2G', '33553920' -> bytes (multiple of 512)."""
    if isinstance(text, int):
        n = text
    else:
        m = _SIZE.match(str(text))
        if not m:
            raise ManifestError("cannot parse size %r (use e.g. 64M, 1440K, 2G or a byte count)" % (text,))
        n = int(float(m.group(1)) * {"": 1, "k": 1024, "m": 1024 ** 2, "g": 1024 ** 3}[m.group(2).lower()])
    if n % 512:
        raise ManifestError("size %r is not a multiple of 512 bytes" % (text,))
    return n


_EPOCH = datetime.datetime(1904, 1, 1)


def mac_from_iso(text):
    """'2026-11-11T20:15:00' (Mac local time) or an integer -> Mac seconds."""
    if text is None:
        return None
    if isinstance(text, (int, float)):
        return int(text) & 0xFFFFFFFF
    try:
        dt = datetime.datetime.strptime(text.replace(" ", "T"), "%Y-%m-%dT%H:%M:%S")
    except ValueError:
        try:
            dt = datetime.datetime.strptime(text, "%Y-%m-%d")
        except ValueError:
            raise ManifestError("bad date %r (use YYYY-MM-DDTHH:MM:SS)" % (text,))
    secs = int((dt - _EPOCH).total_seconds())
    if not 0 <= secs <= 0xFFFFFFFF:
        raise ManifestError("date %r is outside the Mac range 1904..2040" % (text,))
    return secs


def iso_from_mac(secs):
    if not secs:
        return None
    return (_EPOCH + datetime.timedelta(seconds=secs)).strftime("%Y-%m-%dT%H:%M:%S")


def mac_from_unix(ts):
    """Host time stamp -> Mac seconds in *local* time (HFS dates carry no zone)."""
    try:
        dt = datetime.datetime.fromtimestamp(ts)
    except (OverflowError, OSError, ValueError):
        dt = datetime.datetime(1970, 1, 1)
    secs = int((dt.replace(microsecond=0) - _EPOCH).total_seconds())
    return min(max(secs, 0), 0xFFFFFFFF)


def unix_from_mac(secs):
    """Mac local-time seconds -> host time stamp (for os.utime)."""
    dt = _EPOCH + datetime.timedelta(seconds=secs)
    try:
        return dt.timestamp()
    except (OverflowError, OSError, ValueError):
        return secs - MAC_EPOCH_DELTA


def parse_flags(value):
    """Finder flags: int, '0x2000', or a list of names."""
    names = {"hasBundle": 0x2000, "invisible": 0x4000, "isAlias": 0x8000, "hasCustomIcon": 0x0400,
             "isStationery": 0x0800, "nameLocked": 0x1000, "inited": 0x0100, "noInits": 0x0080,
             "shared": 0x0040}
    if value is None:
        return 0
    if isinstance(value, int):
        return value & 0xFFFF
    if isinstance(value, str):
        try:
            return int(value, 0) & 0xFFFF
        except ValueError:
            value = [v.strip() for v in value.split(",") if v.strip()]
    flags = 0
    for v in value:
        if v not in names:
            raise ManifestError("unknown Finder flag %r (known: %s)" % (v, ", ".join(sorted(names))))
        flags |= names[v]
    return flags


def match_rule(rules, relpath):
    """First rule whose pattern matches the posix relative path (case-insensitive).

    A pattern without '/' is matched against the file name only; a pattern with
    '/' against the whole path ('bin/*' = files directly or deeper under bin/).
    """
    rel = relpath.lower()
    base = rel.rsplit("/", 1)[-1]
    for rule in rules:
        pat = str(rule.get("match", "*")).lower()
        if fnmatch.fnmatchcase(rel if "/" in pat else base, pat):
            return rule
    return {}


def ignored(patterns, name):
    low = name.lower()
    return any(fnmatch.fnmatchcase(low, p.lower()) for p in patterns)


def entry_for(man, relpath):
    """Manifest entry for a posix relative path (exact, then case-insensitive)."""
    entries = man.get("entries") or {}
    if relpath in entries:
        return entries[relpath]
    low = relpath.lower()
    for k, v in entries.items():
        if k.lower().rstrip("/") == low:
            return v
    return {}


def type_creator(man, relpath, entry=None):
    entry = entry if entry is not None else entry_for(man, relpath)
    rule = match_rule(man.get("rules") or DEFAULT_RULES, relpath)
    return (fourcc(entry.get("type", rule.get("type", "????"))),
            fourcc(entry.get("creator", rule.get("creator", "????"))))

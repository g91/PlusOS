"""`hdbuild.py stage`: lay the PlusOS volume out as an ordinary host folder.

The folder mirrors the Mac volume 1:1 (bin/, Games/, Desktop/, multiboot.cfg,
anything from --overlay); what a host folder cannot hold goes to
.hdbuild.json, .rsrc/ (resource forks) and .boot/ (loader, kernel, driver).

App selection follows tools/install.py exactly: every build/apps/*.pex, name
upper-cased without extension, filtered by include_apps= of multiboot.cfg
(SHELL, NETDBG and DBGRUN are always kept; programs that have no source
folder under apps/ count as "newly discovered" and are always kept too),
plus custom_apps=NAME:path;... .
"""
import os
import shutil
import stat

from . import forks, macroman, manifest
from .hfs import fourcc_str
from .manifest import MANIFEST_NAME, RSRC_DIR, BOOT_DIR

GAMES = {"ASTEROIDS", "BREAKOUT", "CHIP8", "CUBE3D", "DUNGEON", "FLAPPY",
         "INVADERS", "LUNAR", "MINES", "PACMAN", "PUZZLE", "RACER", "SNAKE",
         "SOLARSYS", "SPIN", "STARFLIGHT", "TETRIS"}
ALWAYS = {"SHELL", "NETDBG", "DBGRUN"}

# tools/make_floppy.py / README "Minimal Bootable Floppy"
FLOPPY_APPS = ["INSTALLER", "SHELL", "DESKTOP", "FILES", "PKG", "PKGMGR", "CAT", "LS", "SYSINFO",
               "TASKMAN", "CALC", "NOTES", "MEM", "VER", "BOOT", "REBOOT", "OFF", "REG"]
PRESETS = {
    "floppy1440": {"size": "1440K", "volume": "PlusOS Install"},
    "floppy800": {"size": "800K", "volume": "PlusOS Install"},
}

DEFAULT_CFG = (
    "# PlusOS Multi-Boot Configuration\r\n"
    "boot_pref=2\r\n\r\n"
    "[entry]\r\nname=System 7.5.5\r\nid=3\r\n\r\n"
    "[entry]\r\nname=System 7.1\r\nid=2\r\n\r\n"
    "[entry]\r\nname=System 6.0.8\r\nid=1\r\n\r\n"
    "[entry]\r\nname=PlusOS Shell\r\nid=shell\r\n\r\n"
    "[entry]\r\nname=PlusOS Desktop\r\nid=desktop\r\n"
)
FLOPPY_CFG = (
    "# PlusOS Floppy Installer Boot Configuration\r\n"
    "boot_pref=1\r\n\r\n"
    "[entry]\r\nname=PlusOS Installer\r\nid=installer\r\n\r\n"
    "[entry]\r\nname=PlusOS Desktop\r\nid=desktop\r\n\r\n"
    "[entry]\r\nname=PlusOS Shell\r\nid=shell\r\n"
)


class StageError(Exception):
    pass


def repo_root():
    return os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))


def read_cfg_filters(cfg_path):
    """include_apps= / custom_apps= of multiboot.cfg, parsed like install.py."""
    included, custom = None, []
    if cfg_path and os.path.exists(cfg_path):
        with open(cfg_path, "r", encoding="latin-1") as cf:
            for line in cf:
                line = line.strip()
                if line.startswith("include_apps="):
                    val = line.split("=", 1)[1].strip()
                    if val:
                        included = set(x.strip().upper() for x in val.split(",") if x.strip())
                elif line.startswith("custom_apps="):
                    val = line.split("=", 1)[1].strip()
                    for entry in val.split(";") if val else []:
                        if ":" in entry:
                            cname, cpath = entry.split(":", 1)
                            cpath = cpath.strip()
                            for cand in (cpath, os.path.join(os.path.dirname(os.path.abspath(cfg_path)), cpath)):
                                if os.path.exists(cand):
                                    custom.append((cname.strip().upper(), cand))
                                    break
    return included, custom


def select_apps(build_dir, cfg_path, repo, extra=(), log=print):
    """-> ordered list of (NAME, path) exactly as install.py would install."""
    apps_dir = os.path.join(build_dir, "apps")
    found = []
    if os.path.isdir(apps_dir):
        for entry in sorted(os.listdir(apps_dir)):
            if entry.endswith(".pex"):
                found.append((os.path.splitext(entry)[0].upper(), os.path.join(apps_dir, entry)))
    # The Makefile passes --app for every apps/<group>/<name>/ source folder;
    # anything else in build/apps is "newly discovered" and bypasses the filter.
    known = set()
    src = os.path.join(repo, "apps")
    if os.path.isdir(src):
        for group in os.listdir(src):
            gdir = os.path.join(src, group)
            if os.path.isdir(gdir):
                known.update(n.upper() for n in os.listdir(gdir))
    else:
        known = {n for n, _ in found}
    discovered = {n for n, _ in found if n not in known}
    included, custom = read_cfg_filters(cfg_path)
    apps = found
    if included is not None:
        keep = included | ALWAYS | discovered
        apps = [a for a in found if a[0] in keep]
        log("  package filter: including %d of %d compiled apps" % (len(apps), len(found)))
        missing = sorted(included - {n for n, _ in found})
        if missing:
            log("  note: include_apps names with no compiled .pex: %s" % ", ".join(missing))
    for name, path in list(custom) + [tuple(x) for x in extra]:
        apps = [a for a in apps if a[0] != name] + [(name, path)]
        log("  custom package added: %s (%s)" % (name, path))
    return apps


# ---- putting files into the staging folder ----------------------------------

def _rel_join(rel, name):
    return (rel + "/" + name) if rel else name


def put_decoded(root, rel, dec, man, keep_dates=True):
    """Store a decoded MacBinary/AppleSingle file at posix path `rel` in the staging folder."""
    target = os.path.join(root, *rel.split("/"))
    os.makedirs(os.path.dirname(target), exist_ok=True)
    with open(target, "wb") as fh:
        fh.write(dec.data or b"")
    entry = man["entries"].setdefault(rel, {})
    apply_meta(root, rel, dec, entry, keep_dates)
    return entry


def apply_meta(root, rel, dec, entry, keep_dates=True):
    if dec.ftype:
        entry["type"] = fourcc_str(dec.ftype)
    if dec.creator:
        entry["creator"] = fourcc_str(dec.creator)
    if dec.flags:
        entry["flags"] = "0x%04X" % dec.flags
    if dec.fxinfo and dec.fxinfo.strip(b"\0"):
        entry["fxinfo"] = dec.fxinfo.hex()
    if dec.locked:
        entry["locked"] = True
    if keep_dates:
        if dec.created:
            entry["created"] = manifest.iso_from_mac(dec.created)
        if dec.modified:
            entry["modified"] = manifest.iso_from_mac(dec.modified)
    if dec.rsrc:
        side = os.path.join(root, RSRC_DIR, *rel.split("/")) + ".rsrc"
        os.makedirs(os.path.dirname(side), exist_ok=True)
        with open(side, "wb") as fh:
            fh.write(dec.rsrc)
        entry["rsrc"] = RSRC_DIR + "/" + rel + ".rsrc"


def copy_overlay(src, root, man, log=print):
    """Copy the tree `src` on top of the staging folder, decoding Mac containers."""
    if not os.path.isdir(src):
        raise StageError("overlay %s is not a folder" % src)
    count = 0
    for cur, dirs, files in os.walk(src):
        dirs.sort()
        relparts = [] if os.path.abspath(cur) == os.path.abspath(src) else \
            os.path.relpath(cur, src).replace("\\", "/").split("/")
        rel = "/".join(relparts)
        os.makedirs(os.path.join(root, *relparts), exist_ok=True)
        for name in sorted(files):
            path = os.path.join(cur, name)
            if not rel and name == MANIFEST_NAME:
                import json
                with open(path, "r", encoding="utf-8") as fh:
                    over = json.load(fh)
                man["entries"].update(over.pop("entries", {}) or {})
                over.pop("hdbuild", None)
                manifest._merge(man, over)
                log("  overlay: merged %s" % path)
                continue
            if relparts[:1] in ([RSRC_DIR], [BOOT_DIR]):
                shutil.copy2(path, os.path.join(root, *(relparts + [name])))
                continue
            with open(path, "rb") as fh:
                head = fh.read(128)
            if name.startswith("._"):
                with open(path, "rb") as fh:
                    dec = forks.decode_applefile(fh.read())
                if dec is not None and name[2:] in files:
                    apply_meta(root, _rel_join(rel, name[2:]), dec,
                               man["entries"].setdefault(_rel_join(rel, name[2:]), {}), keep_dates=False)
                    count += 1
                    continue
            if name.lower().endswith(".bin") and len(head) == 128 and head[0] == 0:
                with open(path, "rb") as fh:
                    dec = forks.decode_macbinary(fh.read())
                if dec is not None:
                    put_decoded(root, _rel_join(rel, name[:-4]), dec, man)
                    log("  overlay: decoded MacBinary %s -> %s" % (path, _rel_join(rel, name[:-4])))
                    count += 1
                    continue
            if head[:4] == b"\x00\x05\x16\x00":
                with open(path, "rb") as fh:
                    dec = forks.decode_applefile(fh.read())
                if dec is not None and dec.data is not None:
                    out = name[:-3] if name.lower().endswith(".as") else name
                    put_decoded(root, _rel_join(rel, out), dec, man)
                    log("  overlay: decoded AppleSingle %s -> %s" % (path, _rel_join(rel, out)))
                    count += 1
                    continue
            shutil.copy2(path, os.path.join(root, *(relparts + [name])))
            count += 1
    log("  overlay: %d files from %s" % (count, src))


def _rm(fn, path):
    """Remove, clearing a read-only attribute first if that is what blocks us (Windows)."""
    try:
        fn(path)
    except PermissionError:
        rwx = stat.S_IWRITE | stat.S_IREAD | stat.S_IEXEC
        for p in (os.path.dirname(path), path):       # POSIX: the parent must be writable
            try:
                os.chmod(p, rwx)
            except OSError:
                pass
        fn(path)


def remove_tree(path):
    """shutil.rmtree that also copes with read-only files and folders."""
    if os.path.islink(path) or not os.path.isdir(path):
        _rm(os.unlink, path)
        return
    for cur, dirs, files in os.walk(path, topdown=False):
        for name in files:
            _rm(os.unlink, os.path.join(cur, name))
        for name in dirs:
            p = os.path.join(cur, name)
            _rm(os.unlink if os.path.islink(p) else os.rmdir, p)
    _rm(os.rmdir, path)


def _clean_root(root, force):
    if os.path.isdir(root) and os.listdir(root):
        if not os.path.exists(os.path.join(root, MANIFEST_NAME)) and not force:
            raise StageError("%s exists, is not empty and is not a staging folder (no %s); "
                             "refusing to wipe it (use --force)" % (root, MANIFEST_NAME))
        for name in os.listdir(root):
            remove_tree(os.path.join(root, name))
    os.makedirs(root, exist_ok=True)


def _eol(data, mode):
    if mode in (None, "keep"):
        return data
    text = data.replace(b"\r\n", b"\n").replace(b"\r", b"\n")
    return text.replace(b"\n", {"cr": b"\r", "lf": b"\n", "crlf": b"\r\n"}[mode])


def stage(opts, log=print):
    """Create the staging folder.  `opts` is a dict (see cli.py).  Returns the manifest."""
    repo = opts.get("repo") or repo_root()
    preset = opts.get("preset")
    if preset and preset not in PRESETS:
        raise StageError("unknown preset %r (known: %s)" % (preset, ", ".join(sorted(PRESETS))))
    build_dir = opts.get("build") or os.path.join(repo, "build")
    root = opts.get("root") or os.path.join(build_dir, "floppy_root" if preset else "disk_root")
    cfg_path = opts.get("config") or os.path.join(repo, "multiboot.cfg")
    kernel = opts.get("kernel") or os.path.join(build_dir, "kernel", "kernel.pex")
    loader = opts.get("loader")
    if not loader:
        loader = os.path.join(build_dir, "boot", "loader_floppy.bin" if preset else "loader.bin")
        if preset and not os.path.exists(loader):
            raise StageError(
                "%s not found.  A floppy needs a loader assembled for the floppy kernel offset "
                "(see the Makefile `floppy` target: -DKERNEL_OFFSET=51200); the hard-disk loader.bin "
                "would look for the kernel at block 60000.  Build it, or pass --loader FILE." % loader)
    for what, path in (("boot loader", loader), ("kernel", kernel)):
        if not os.path.isfile(path):
            raise StageError("%s %s not found - build PlusOS first (build.bat / make)" % (what, path))

    _clean_root(root, opts.get("force"))
    man = manifest.default_manifest()
    man["machine"] = opts.get("machine") or "se"
    log("staging %s" % root)

    # ---- programs ----
    if preset:
        apps = []
        apps_dir = os.path.join(build_dir, "apps")
        for name in FLOPPY_APPS:
            for cand in (name.lower() + ".pex", name + ".pex"):
                if os.path.exists(os.path.join(apps_dir, cand)):
                    apps.append((name, os.path.join(apps_dir, cand)))
                    break
            else:
                log("  warning: essential app %s has no compiled .pex" % name)
    else:
        apps = select_apps(build_dir, cfg_path, repo, opts.get("apps") or (), log)
    if not apps:
        log("  warning: no compiled apps found under %s" % os.path.join(build_dir, "apps"))
    os.makedirs(os.path.join(root, "bin"), exist_ok=True)
    for name, path in apps:
        macroman.encode(name, strict=True)
        shutil.copy2(path, os.path.join(root, "bin", name))
    log("  bin/: %d programs" % len(apps))
    if not preset:
        games = [(n, p) for n, p in apps if n in GAMES]
        os.makedirs(os.path.join(root, "Games"), exist_ok=True)
        for name, path in games:
            shutil.copy2(path, os.path.join(root, "Games", name))
        log("  Games/: %d programs" % len(games))

    # ---- multiboot.cfg ----
    cfg_out = os.path.join(root, "multiboot.cfg")
    if preset:
        with open(cfg_out, "wb") as fh:
            fh.write(FLOPPY_CFG.encode("ascii"))
        log("  multiboot.cfg: floppy installer profile")
    elif os.path.exists(cfg_path):
        with open(cfg_path, "rb") as fh:
            data = fh.read()
        with open(cfg_out, "wb") as fh:
            fh.write(_eol(data, opts.get("cfg_eol")))
        shutil.copystat(cfg_path, cfg_out)
        log("  multiboot.cfg: from %s" % cfg_path)
    else:
        with open(cfg_out, "wb") as fh:
            fh.write(DEFAULT_CFG.encode("ascii"))
        log("  multiboot.cfg: seeded default")

    # ---- Desktop ----
    os.makedirs(os.path.join(root, "Desktop"), exist_ok=True)
    if preset:
        inst = next((p for n, p in apps if n == "INSTALLER"), None)
        if inst:
            shutil.copy2(inst, os.path.join(root, "Desktop", "Installer"))
            man["entries"]["Desktop/Installer"] = {"type": "PLOS", "creator": "PLOS"}
            log("  Desktop/Installer")
    ret = opts.get("return_app") or os.path.join(repo, "tools", "return_to_plusos", "Return to PlusOS.bin")
    if os.path.exists(ret):
        with open(ret, "rb") as fh:
            dec = forks.decode_macbinary(fh.read())
        if dec is None:
            log("  warning: %s is not a MacBinary file; skipped" % ret)
        else:
            put_decoded(root, "Desktop/Return to PlusOS", dec, man)
            log("  Desktop/Return to PlusOS (%s/%s, resource fork %d bytes)"
                % (fourcc_str(dec.ftype), fourcc_str(dec.creator), len(dec.rsrc)))
    else:
        log("  note: %s not found; Desktop/Return to PlusOS not staged" % ret)

    # ---- boot loader, kernel, raw areas ----
    os.makedirs(os.path.join(root, BOOT_DIR), exist_ok=True)
    shutil.copy2(loader, os.path.join(root, BOOT_DIR, "loader.bin"))
    shutil.copy2(kernel, os.path.join(root, BOOT_DIR, "kernel.pex"))
    log("  %s/loader.bin (%d bytes), %s/kernel.pex (%d bytes)"
        % (BOOT_DIR, os.path.getsize(loader), BOOT_DIR, os.path.getsize(kernel)))
    kc = man["raw"]["kernel"]
    if preset:
        p = PRESETS[preset]
        man["image"] = {"format": "dsk", "size": p["size"], "driver": None}
        man["volume"]["name"] = p["volume"]
        kc["block"], kc["max_blocks"] = manifest.FLOPPY_KERNEL_BLOCK, manifest.FLOPPY_KERNEL_BLOCKS
        man["raw"]["apps"] = None
    else:
        raw_apps = apps
        if len(apps) > 85:
            # The raw app directory fits at most 85 entries; keep the core
            # recovery/system/known set in raw.apps while all executables live in HFS :bin:.
            included, _ = read_cfg_filters(cfg_path)
            core_set = (included or set()) | ALWAYS | set(FLOPPY_APPS)
            raw_apps = [a for a in apps if a[0] in core_set][:85]
        man["raw"]["apps"]["entries"] = [{"name": n, "path": "bin/" + n} for n, _ in raw_apps]
    if opts.get("kernel_block") is not None:
        kc["block"] = int(opts["kernel_block"])
    if opts.get("kernel_blocks"):
        kc["max_blocks"] = int(opts["kernel_blocks"])
    if opts.get("apps_block") is not None and man["raw"]["apps"]:
        man["raw"]["apps"]["block"] = int(opts["apps_block"])
    if opts.get("no_raw_apps"):
        man["raw"]["apps"] = None
    if opts.get("volume_name"):
        man["volume"]["name"] = opts["volume_name"]
    if opts.get("size"):
        man["image"]["size"] = opts["size"]
    if opts.get("format"):
        man["image"]["format"] = opts["format"]
        if opts["format"] == "dsk":
            man["image"].pop("hfs_start", None)
            man["image"].pop("hfs_partition_name", None)
    if opts.get("driver"):
        dst = os.path.join(root, BOOT_DIR, "driver.bin")
        shutil.copy2(opts["driver"], dst)
        meta = {"path": BOOT_DIR + "/driver.bin"}
        if os.path.exists(opts["driver"] + ".json"):
            import json
            with open(opts["driver"] + ".json", "r", encoding="utf-8") as fh:
                meta.update(json.load(fh))
            meta["path"] = BOOT_DIR + "/driver.bin"
        man["image"]["driver"] = meta
        log("  %s/driver.bin (%d bytes)" % (BOOT_DIR, os.path.getsize(dst)))

    for ov in opts.get("overlay") or ():
        copy_overlay(ov, root, man, log)

    manifest.save(root, man)
    log("  %s written" % MANIFEST_NAME)
    return man

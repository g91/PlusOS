"""Command line front end of hdbuild."""
import argparse
import json
import os
import sys

from . import apm, fsck, image, macroman, manifest, stage as stage_mod, __version__
from .hfs import SECTOR, HFSError, fourcc_str

EPILOG = """\
examples:
  hdbuild.py build --out images/PlusOS.hda            stage build/disk_root + build a 64M hard disk
  hdbuild.py stage --overlay extras/                  only lay out build/disk_root (edit it by hand)
  hdbuild.py image --root build/disk_root --out images/PlusOS.hda --size 128M
  hdbuild.py build --preset floppy1440 --out images/PlusOS_Floppy.dsk
  hdbuild.py build --from-template "F:/HD60_512 PlusOS.hda" --out images/PlusOS.hda
  hdbuild.py verify images/PlusOS.hda                 fsck the image
  hdbuild.py ls -lR images/PlusOS.hda bin
  hdbuild.py extract images/PlusOS.hda build/extracted
"""


class CliError(Exception):
    pass


def _stage_args(p):
    g = p.add_argument_group("staging")
    g.add_argument("--build", metavar="DIR", help="build output folder (default: <repo>/build)")
    g.add_argument("--config", metavar="FILE", help="multiboot.cfg to install and to read "
                   "include_apps=/custom_apps= from (default: <repo>/multiboot.cfg)")
    g.add_argument("--machine", default="se", help="target machine, recorded in the manifest (default: se)")
    g.add_argument("--overlay", metavar="DIR", action="append", default=[],
                   help="extra tree copied on top of the volume (repeatable); MacBinary .bin, "
                        "AppleSingle and AppleDouble ._name files are decoded")
    g.add_argument("--app", metavar="NAME=FILE", action="append", default=[],
                   help="add/replace one program in bin/ (repeatable)")
    g.add_argument("--preset", choices=sorted(stage_mod.PRESETS),
                   help="minimal installer floppy (bare .dsk, essential apps only, kernel at block 100)")
    g.add_argument("--cfg-eol", choices=["keep", "cr", "lf", "crlf"], default="keep",
                   help="line endings of the staged multiboot.cfg (default: keep, like install.py)")
    g.add_argument("--force", action="store_true", help="wipe --root even if it is not a staging folder")


def _image_args(p, staged=False):
    g = p.add_argument_group("image")
    g.add_argument("--size", help="disk size, e.g. 64M, 128M, 1440K (default: manifest, 64M)")
    g.add_argument("--volume-name", help="HFS volume name (default: manifest, PlusOS)")
    g.add_argument("--format", choices=["hda", "dsk"],
                   help="hda = DDR + Apple partition map, dsk = bare HFS volume (default: manifest, hda)")
    g.add_argument("--driver", metavar="FILE",
                   help="SCSI driver blob to embed in an Apple_Driver43 partition "
                        "(FILE.json metadata from extract-driver is used if present)")
    g.add_argument("--driver-type", help="partition type of the driver (default Apple_Driver43)")
    g.add_argument("--driver-bootargs", metavar="HEX", help="boot-args bytes after pmProcessor")
    g.add_argument("--driver-from", metavar="DIR_OR_IMAGE",
                   help="auto-source an Apple SCSI driver: if an image, extract its driver; if a "
                        "directory (e.g. a BlueSCSI SD card), scan its *.hda for the first one that "
                        "has a driver and reuse it. Lets the image boot on real hardware without "
                        "naming a driver file. Ignored if --driver or --from-template is given.")
    g.add_argument("--from-template", metavar="IMAGE",
                   help="copy DDR, partition map and driver partition from a working disk image and "
                        "rebuild only its Apple_HFS partition")
    g.add_argument("--hfs-start", type=int, help="first block of the Apple_HFS partition (default 96)")
    g.add_argument("--alloc-block-size", type=int, help="force the allocation block size (bytes)")
    g.add_argument("--loader", metavar="FILE", help="boot block loader (default: build/boot/loader.bin)")
    g.add_argument("--kernel", metavar="FILE", help="kernel image (default: build/kernel/kernel.pex)")
    g.add_argument("--kernel-block", type=int, help="raw kernel block (default 60000; floppy 100)")
    g.add_argument("--kernel-blocks", type=int, help="max kernel size in blocks (default 512; floppy 360)")
    g.add_argument("--apps-block", type=int, help="raw app store block (default 61000)")
    g.add_argument("--no-raw-apps", action="store_true", help="do not write the raw app store")
    g.add_argument("--strict-names", action="store_true",
                   help="fail on names MacRoman cannot represent instead of transliterating")
    g.add_argument("--no-verify", action="store_true", help="skip the verify pass after building")


def _opts(a, keys):
    return {k: getattr(a, k) for k in keys if getattr(a, k, None) not in (None, [], False)}


STAGE_KEYS = ["build", "root", "config", "machine", "overlay", "preset", "cfg_eol", "force", "loader",
              "kernel", "kernel_block", "kernel_blocks", "apps_block", "no_raw_apps", "volume_name",
              "size", "format", "driver"]
IMAGE_KEYS = ["size", "volume_name", "format", "driver", "driver_type", "driver_bootargs",
              "from_template", "hfs_start", "alloc_block_size", "loader", "kernel", "kernel_block",
              "kernel_blocks", "apps_block", "no_raw_apps", "strict_names"]


def _do_stage(a):
    o = _opts(a, STAGE_KEYS)
    apps = []
    for spec in a.app:
        if "=" not in spec:
            raise CliError("--app wants NAME=FILE, got %r" % spec)
        name, path = spec.split("=", 1)
        apps.append((name.upper(), path))
    o["apps"] = apps
    stage_mod.stage(o)
    repo = stage_mod.repo_root()
    build = o.get("build") or os.path.join(repo, "build")
    return o.get("root") or os.path.join(build, "floppy_root" if o.get("preset") else "disk_root")


def _do_image(a, root, staged=False):
    o = _opts(a, IMAGE_KEYS)
    if staged:
        # the staging step already put these into the folder / manifest
        for k in ("loader", "kernel", "kernel_block", "kernel_blocks", "apps_block", "no_raw_apps",
                  "volume_name", "format"):
            o.pop(k, None)
    out = a.out
    if not out:
        man = manifest.load(root)
        ext = "dsk" if (o.get("format") or man["image"].get("format")) == "dsk" else "hda"
        name = "PlusOS_Floppy.dsk" if ext == "dsk" else "PlusOS.hda"
        out = os.path.join(stage_mod.repo_root(), "images", name)
    info = image.build_image(root, out, o)
    image.describe_build(info)
    if not a.no_verify:
        rep = fsck.verify(out, expect_raw=bool(info["raw"]))
        _print_report(rep, quiet=True)
        if not rep.ok:
            raise CliError("verify FAILED for %s" % out)
        print("  verify: OK")
    return 0


def cmd_stage(a):
    _do_stage(a)
    return 0


def cmd_image(a):
    _resolve_driver_from(a)
    return _do_image(a, a.root or os.path.join(stage_mod.repo_root(), "build", "disk_root"))


def _image_has_driver(path):
    """Return True if `path` is a disk image with an Apple_Driver partition."""
    try:
        with image.Opened(path) as img:
            return bool(img.apm and apm.find_driver(img.apm))
    except Exception:
        return False


def _resolve_driver_from(a):
    """Honour --driver-from: turn a directory or image into a concrete driver source.
    Sets a.from_template (reuse an image's DDR/map/driver) unless --driver / --from-template
    already chose one. Scans a directory (e.g. a BlueSCSI SD card) for the first *.hda that
    carries a driver, preferring larger/newer system disks over PlusOS's own output."""
    src = getattr(a, "driver_from", None)
    if not src or getattr(a, "driver", None) or getattr(a, "from_template", None):
        return
    # a bare Windows drive letter ("F:" or "F:\") means that drive's root directory
    if len(src) == 2 and src[1] == ":":
        src = src + os.sep
    if os.path.isdir(src):
        cands = []
        for name in os.listdir(src):
            if not name.lower().endswith((".hda", ".img", ".dsk")):
                continue
            p = os.path.join(src, name)
            # skip the PlusOS image we are about to (re)write and tiny network stubs
            if "plusos" in name.lower():
                continue
            try:
                sz = os.path.getsize(p)
            except OSError:
                continue
            if sz < 2 * 1024 * 1024:
                continue
            if _image_has_driver(p):
                cands.append((sz, p))
        if not cands:
            raise CliError("--driver-from %s: no disk image with an Apple SCSI driver found there "
                           "(put a bootable Mac OS .hda on the card, or use --driver / --from-template)" % src)
        # prefer the largest driver-bearing system disk (most likely a full Mac OS install)
        cands.sort()
        chosen = cands[-1][1]
        print("  driver: auto-sourced from %s" % chosen)
        a.from_template = chosen
    elif os.path.isfile(src):
        if _image_has_driver(src):
            print("  driver: using template image %s" % src)
            a.from_template = src
        else:
            # treat it as a raw driver blob
            print("  driver: using blob %s" % src)
            a.driver = src
    else:
        raise CliError("--driver-from %s: not a file or directory" % src)


def cmd_build(a):
    _resolve_driver_from(a)
    root = _do_stage(a)
    return _do_image(a, root, staged=True)


def _date(secs):
    return (manifest.iso_from_mac(secs) or "-").replace("T", " ")


def cmd_ls(a):
    with image.Opened(a.image) as img:
        vol = img.volume()
        start = vol.lookup(a.path or "")
        if start is None:
            raise CliError("%s: no such file or folder in volume '%s'" % (a.path, vol.uname))

        def line(e):
            if not a.long:
                return e.uname + (":" if e.is_dir else "")
            if e.is_dir:
                return "d %-9s %9s %9d %s  %s" % ("", "", len(vol.listdir(e)), _date(e.modified), e.uname)
            return "f %4s/%4s %9d %9d %s  %s%s" % (fourcc_str(e.ftype), fourcc_str(e.creator), e.rlen, e.dlen,
                                               _date(e.modified), e.uname, "  (locked)" if e.locked else "")

        def show(d, label):
            if a.recursive:
                print("%s:" % (label or ":"))
            for e in vol.listdir(d):
                print(line(e))
            if a.recursive:
                print("")
                for e in vol.listdir(d):
                    if e.is_dir:
                        show(e, label + ":" + e.uname)

        if not start.is_dir:
            print(line(start))
        else:
            if a.long and not a.recursive:
                print("  type/crea      rsrc      data modified             name")
            show(start, "" if start.cnid == 2 else ":" + start.uname)
    return 0


def cmd_info(a):
    with image.Opened(a.image) as img:
        print("%s: %d bytes (%d blocks)" % (a.image, img.size, img.size // SECTOR))
        if img.apm:
            i = img.apm
            print("Driver Descriptor Record: block size %d, %d blocks, devType %d, devId %d, %d driver(s)"
                  % (i["block_size"], i["block_count"], i["dev_type"], i["dev_id"], len(i["drivers"])))
            for blk, size, typ in i["drivers"]:
                print("  driver: block %d, %d blocks, type %d" % (blk, size, typ))
            print("Partition map:")
            print("   #  start     blocks     status    type                  name")
            for p in i["partitions"]:
                extra = ""
                if p["boot_size"]:
                    extra = "  boot %d bytes cksum $%04X cpu %s" % (p["boot_size"], p["boot_cksum"], p["processor"])
                print("  %2d  %-9d %-10d $%08X %-21s %s%s" % (p["index"], p["start"], p["blocks"], p["status"],
                                                             p["type"], p["name"], extra))
            if not apm.find_driver(i):
                print("  (no Apple_Driver partition: emulator-only image, will not boot on real SCSI hardware)")
        else:
            print("No partition map: bare HFS volume")
        vol = img.volume()
        m = vol.mdb
        print("HFS volume '%s' at block %d, %d blocks" % (vol.uname, img.base, img.nsect))
        print("  created %s, modified %s" % (_date(m["drCrDate"]), _date(m["drLsMod"])))
        print("  allocation blocks: %d x %d bytes, first at sector %d (drAlBlSt), bitmap at sector %d"
              % (vol.nmal, vol.alblk, vol.alst, m["drVBMSt"]))
        print("  free: %d blocks = %d bytes (%.1f MB) of %.1f MB"
              % (m["drFreeBks"], vol.free_bytes(), vol.free_bytes() / 1048576.0, vol.nmal * vol.alblk / 1048576.0))
        print("  files %d, folders %d, root: %d files + %d folders, next CNID %d, write count %d"
              % (m["drFilCnt"], m["drDirCnt"], m["drNmFls"], m["drNmRtDirs"], m["drNxtCNID"], m["drWrCnt"]))
        print("  attributes $%04X, clump %d, alloc ptr %d, blessed folder %d"
              % (m["drAtrb"], m["drClpSiz"], m["drAllocPtr"], int.from_bytes(m["drFndrInfo"][:4], "big")))
        for label, tree, size, ext, clp in (
                ("extents", vol.extents_tree, m["drXTFlSize"], vol.xt_ext, m["drXTClpSiz"]),
                ("catalog", vol.catalog_tree, m["drCTFlSize"], vol.ct_ext, m["drCTClpSiz"])):
            print("  %s B-tree: %d bytes (clump %d), extents %s, depth %d, root %d, %d records, "
                  "%d of %d nodes free" % (label, size, clp, [e for e in ext if e[1]], tree.depth, tree.root,
                                           tree.nrecs, tree.free, tree.nnodes))
        boot = vol.read_sectors(0, 2)
        print("  boot blocks: %s" % ("'LK' loader, %d bytes used" % len(boot.rstrip(b"\0")) if boot[:2] == b"LK"
                                    else ("empty" if not boot.strip(b"\0") else "unknown content")))
        raw = image.detect_raw(vol, kernel_block=a.kernel_block, apps_block=a.apps_block)
        if raw:
            k = raw["kernel"]
            s0, s1 = raw["run"]
            print("PlusOS raw areas (partition-relative blocks):")
            print("  kernel: block %d, %d bytes%s" % (k["block"], k["length"],
                                                      "" if k["reserved"] else "  ** NOT reserved in the bitmap **"))
            if k["reserved"]:
                print("  reserved in the volume bitmap: blocks %d-%d" % (s0, s1))
            if "apps" in raw:
                ap = raw["apps"]
                print("  app store: directory at block %d, %d programs" % (ap["block"], len(ap["entries"])))
                if a.verbose:
                    for name, start, length in ap["entries"]:
                        print("    %-16s block %-6d %7d bytes" % (name, start, length))
        else:
            print("No PlusOS raw kernel area found")
    return 0


def cmd_extract(a):
    image.extract(a.image, a.dest, force=a.force, kernel_block=a.kernel_block, apps_block=a.apps_block)
    return 0


def _print_report(rep, quiet=False):
    if not quiet:
        for n in rep.notes:
            print("  " + n)
    for w in rep.warnings:
        print("  warning: " + w)
    for e in rep.errors:
        print("  ERROR: " + e)


def cmd_verify(a):
    rc = 0
    for path in a.image:
        print(path)
        rep = fsck.verify(path, kernel_block=a.kernel_block, apps_block=a.apps_block,
                          expect_raw=True if a.expect_raw else None)
        _print_report(rep)
        if rep.ok and not (a.strict and rep.warnings):
            print("  OK%s" % (" (%d warnings)" % len(rep.warnings) if rep.warnings else ""))
        else:
            print("  FAILED: %d errors, %d warnings" % (len(rep.errors), len(rep.warnings)))
            rc = 1
    return rc


def cmd_extract_driver(a):
    with image.Opened(a.image) as img:
        if not img.apm:
            raise CliError("%s has no partition map" % a.image)
        drv = apm.find_driver(img.apm)
        if not drv:
            raise CliError("%s has no Apple_Driver partition" % a.image)
        img.f.seek(drv["start"] * SECTOR)
        blob = img.f.read(drv["blocks"] * SECTOR)
        keep = max(drv["boot_size"], len(blob.rstrip(b"\0")))
        with open(a.out, "wb") as fh:
            fh.write(blob[:keep])
        dd = next((d for d in img.apm["drivers"] if d[0] == drv["start"]), None)
        meta = {"type": drv["type"], "name": drv["name"], "processor": drv["processor"],
                "bootargs": drv["bootargs"], "boot_size": drv["boot_size"], "blocks": drv["blocks"],
                "status": drv["status"], "dd_type": dd[2] if dd else 1}
        with open(a.out + ".json", "w", encoding="utf-8") as fh:
            json.dump(meta, fh, indent=2)
            fh.write("\n")
        ck = apm.driver_checksum(blob[:drv["boot_size"]])
        print("%s: %s '%s', %d bytes (boot size %d, checksum $%04X %s) -> %s (+ .json)"
              % (a.image, drv["type"], drv["name"], keep, drv["boot_size"], ck,
                 "ok" if ck == drv["boot_cksum"] else "MISMATCH, map says $%04X" % drv["boot_cksum"], a.out))
    return 0


def main(argv=None):
    ap = argparse.ArgumentParser(
        prog="hdbuild.py", formatter_class=argparse.RawDescriptionHelpFormatter, epilog=EPILOG,
        description="PlusOS hard-disk / floppy image builder: stage the volume as a folder, then build "
                    "an HFS disk image from scratch (no hfsutils, no pre-made image).")
    ap.add_argument("--version", action="version", version="hdbuild " + __version__)
    sub = ap.add_subparsers(dest="cmd")

    p = sub.add_parser("stage", help="lay the volume out as a host folder (+ .hdbuild.json)")
    p.add_argument("--root", metavar="DIR", help="staging folder (default: build/disk_root)")
    _stage_args(p)
    g = p.add_argument_group("recorded in the manifest")
    g.add_argument("--loader", metavar="FILE")
    g.add_argument("--kernel", metavar="FILE")
    g.add_argument("--kernel-block", type=int)
    g.add_argument("--kernel-blocks", type=int)
    g.add_argument("--apps-block", type=int)
    g.add_argument("--no-raw-apps", action="store_true")
    g.add_argument("--volume-name")
    g.add_argument("--size")
    g.add_argument("--format", choices=["hda", "dsk"])
    g.add_argument("--driver", metavar="FILE")
    p.set_defaults(fn=cmd_stage)

    p = sub.add_parser("image", help="build a disk image from a staged folder")
    p.add_argument("--root", metavar="DIR", help="staging folder (default: build/disk_root)")
    p.add_argument("--out", "-o", metavar="IMAGE", help="output image (default: images/PlusOS.hda)")
    _image_args(p)
    p.set_defaults(fn=cmd_image)

    p = sub.add_parser("build", help="stage + image in one go (what `make install` should call)")
    p.add_argument("--root", metavar="DIR", help="staging folder (default: build/disk_root)")
    p.add_argument("--out", "-o", metavar="IMAGE", help="output image (default: images/PlusOS.hda)")
    _stage_args(p)
    _image_args(p)
    p.set_defaults(fn=cmd_build)

    p = sub.add_parser("ls", help="list files in an image")
    p.add_argument("image")
    p.add_argument("path", nargs="?", default="", help="folder or file, e.g. bin or :bin:SHELL")
    p.add_argument("-l", "--long", action="store_true")
    p.add_argument("-R", "--recursive", action="store_true")
    p.set_defaults(fn=cmd_ls)

    def rawargs(p):
        p.add_argument("--kernel-block", type=int, help="where to look for the raw kernel (default: auto)")
        p.add_argument("--apps-block", type=int, help="where to look for the raw app store (default: auto)")

    p = sub.add_parser("info", help="show DDR, partition map, MDB, free space, raw areas")
    p.add_argument("image")
    p.add_argument("-v", "--verbose", action="store_true", help="also list the raw app directory")
    rawargs(p)
    p.set_defaults(fn=cmd_info)

    p = sub.add_parser("extract", help="image -> staged folder + manifest (inverse of `image`)")
    p.add_argument("image")
    p.add_argument("dest")
    p.add_argument("--force", action="store_true", help="extract into a non-empty folder")
    rawargs(p)
    p.set_defaults(fn=cmd_extract)

    p = sub.add_parser("verify", help="check an image for consistency (fsck)")
    p.add_argument("image", nargs="+")
    p.add_argument("--strict", action="store_true", help="treat warnings as failures")
    p.add_argument("--expect-raw", action="store_true", help="fail if there is no PlusOS raw kernel")
    rawargs(p)
    p.set_defaults(fn=cmd_verify)

    p = sub.add_parser("extract-driver", help="save the SCSI driver partition of an image for --driver")
    p.add_argument("image")
    p.add_argument("out", help="output file (metadata goes to OUT.json)")
    p.set_defaults(fn=cmd_extract_driver)

    a = ap.parse_args(argv)
    if not a.cmd:
        ap.print_help()
        return 2
    try:
        sys.stdout.reconfigure(errors="replace")
    except (AttributeError, ValueError):
        pass
    try:
        return a.fn(a) or 0
    except (CliError, image.BuildError, stage_mod.StageError, manifest.ManifestError, HFSError,
            macroman.NameError_, apm.APMError) as e:
        print("hdbuild: error: %s" % e, file=sys.stderr)
        return 1
    except OSError as e:
        print("hdbuild: error: %s" % e, file=sys.stderr)
        return 1

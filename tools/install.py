#!/usr/bin/env python3
"""
install.py - write PlusOS onto BlueSCSI / Mini vMac disk images (.hda).

    install.py --image IMG [--image IMG2 ...] --loader loader.bin
               --kernel kernel.pex [--app NAME=file.pex ...]
               [--kernel-block 60000] [--apps-block 61000] [--hfs]

Layout inside the first Apple_HFS partition (block numbers are partition
relative, 512 bytes each):
    0-1                     boot blocks  -> PlusOS loader
    KERNEL_BLOCK ...        kernel image (raw)
    APPS_BLOCK              app directory (raw fallback store)
    APPS_BLOCK+1 ...        app images
The raw area is marked allocated in the HFS volume bitmap, so the Mac (and
any tool that copies files onto the volume) never puts files on top of it.
With --hfs, apps are also copied into the volume as ordinary files (via
hfsutils), which is how the kernel normally finds them.
"""
import argparse
import os
import shutil
import struct
import subprocess
import sys

BLK = 512
RESERVE_END_OFFSET = 3000          # apps area: APPS_BLOCK .. APPS_BLOCK+3000
DIR_MAGIC = b"PLDR"


def find_hfs_partition(f):
    f.seek(0)
    ddr = f.read(BLK)
    if ddr[:2] != b"ER":
        raise SystemExit("no Apple Driver Descriptor (ER) - not a partitioned disk image")
    entries = None
    n = 1
    while True:
        f.seek(n * BLK)
        pm = f.read(BLK)
        if pm[:2] != b"PM":
            break
        if entries is None:
            entries = struct.unpack_from(">I", pm, 4)[0]
        start, count = struct.unpack_from(">II", pm, 8)
        name = pm[16:48].split(b"\0")[0].decode("mac_roman")
        ptype = pm[48:80].split(b"\0")[0].decode("mac_roman")
        if ptype == "Apple_HFS":
            return n, start, count, name
        n += 1
        if entries and n > entries:
            break
    raise SystemExit("no Apple_HFS partition found")


class Volume:
    def __init__(self, f, start, count):
        self.f, self.start, self.count = f, start, count
        self.mdb = bytearray(self.read(2, 1))
        if self.mdb[:2] != b"BD":
            raise SystemExit("partition has no HFS master directory block")
        (self.vbm_st, self.alloc_ptr, self.nm_al_blks, self.al_blk_siz, self.clp_siz,
         self.al_bl_st, self.nxt_cnid, self.free_bks) = struct.unpack_from(">HHHIIHIH", self.mdb, 14)
        self.name = bytes(self.mdb[37:37 + self.mdb[36]]).decode("mac_roman")

    def read(self, blk, n):
        self.f.seek((self.start + blk) * BLK)
        return self.f.read(n * BLK)

    def write(self, blk, data):
        if len(data) % BLK:
            data = data + b"\0" * (BLK - len(data) % BLK)
        self.f.seek((self.start + blk) * BLK)
        self.f.write(data)

    def reserve(self, first_blk, last_blk):
        """Mark the allocation blocks covering [first_blk, last_blk] as used."""
        per = self.al_blk_siz // BLK
        a0 = (first_blk - self.al_bl_st) // per
        a1 = (last_blk - self.al_bl_st) // per
        if a0 < 0 or a1 >= self.nm_al_blks:
            raise SystemExit("reserved area %d-%d lies outside the HFS volume (%d alloc blocks)"
                             % (first_blk, last_blk, self.nm_al_blks))
        nbytes = (self.nm_al_blks + 7) // 8
        nblk = (nbytes + BLK - 1) // BLK
        bm = bytearray(self.read(self.vbm_st, nblk))
        already = newly = 0
        for a in range(a0, a1 + 1):
            byte, bit = a >> 3, 0x80 >> (a & 7)
            if bm[byte] & bit:
                already += 1
            else:
                bm[byte] |= bit
                newly += 1
        if newly:
            self.write(self.vbm_st, bytes(bm))
            self.free_bks -= newly
            struct.pack_into(">H", self.mdb, 34, self.free_bks)
            self.write(2, bytes(self.mdb))
            alt = self.count - 2                      # alternate MDB
            amdb = bytearray(self.read(alt, 1))
            if amdb[:2] == b"BD":
                struct.pack_into(">H", amdb, 34, self.free_bks)
                self.write(alt, bytes(amdb))
        return a1 - a0 + 1, already, newly


def hfs_copy(image, partno, files):
    """Copy (name, path) files into the volume with hfsutils."""
    tool = os.environ.get("HFSUTILS", "")
    hmount = shutil.which("hmount") or (tool and os.path.join(tool, "hmount"))
    if not hmount or not os.path.exists(hmount):
        print("  hfs: hfsutils not found, skipping HFS copy")
        return False
    bindir = os.path.dirname(hmount)
    run = lambda *a: subprocess.run([os.path.join(bindir, a[0])] + list(a[1:]),
                                    capture_output=True, text=True)
    # hfsutils numbers partitions by counting HFS partitions, not map entries
    for candidate in (1, partno, 0):
        r = run("hmount", image, str(candidate))
        if r.returncode == 0:
            break
    if r.returncode:
        print("  hfs: hmount failed:", (r.stderr or r.stdout).strip())
        return False
    try:
        run("hmkdir", ":bin")                          # the shell's program folder
        run("hmkdir", ":Desktop")                      # files shown on the desktop
        have = run("hls", "-1", ":Desktop").stdout.split()
        seed = dict(files)
        if "HELLO" not in have and "HELLO" in seed:     # starter items, first install only
            run("hcopy", "-r", seed["HELLO"], ":Desktop:HELLO")
            run("hattrib", "-t", "PLOS", "-c", "PLOS", ":Desktop:HELLO")
            print("  hfs: seeded Desktop/HELLO")
        if "Read Me" not in " ".join(have):
            import tempfile
            note = ("Welcome to PlusOS.\r\r"
                    "Drag icons to move them; PlusOS remembers where you put them.\r"
                    "Select an icon and use the Edit menu to Copy, Paste, Duplicate,\r"
                    "Rename or Delete it, or drag it onto the Trash.\r"
                    "Open this file with File > Open in Hex Editor.\r")
            with tempfile.NamedTemporaryFile("w", suffix=".txt", delete=False, newline="") as t:
                t.write(note)
            run("hcopy", "-r", t.name, ":Desktop:Read Me")
            run("hattrib", "-t", "TEXT", "-c", "PLOS", ":Desktop:Read Me")
            os.unlink(t.name)
            print("  hfs: seeded Desktop/Read Me")
        for name, path in files:
            run("hdel", ":" + name)                    # remove stale root copy from older installs
            dst = ":bin:" + name
            run("hdel", dst)                           # replace if present
            r = run("hcopy", "-r", path, dst)
            if r.returncode:
                print("  hfs: hcopy %s failed: %s" % (name, r.stderr.strip()))
            else:
                run("hattrib", "-t", "PLOS", "-c", "PLOS", dst)
                print("  hfs: copied bin/%s" % name)
        # the Games folder: a copy of each game app, shown by the desktop's
        # Games folder and browsable in the Files explorer
        games = {"ASTEROIDS", "BREAKOUT", "CHIP8", "CUBE3D", "DUNGEON", "FLAPPY",
                 "INVADERS", "LUNAR", "MINES", "PACMAN", "PUZZLE", "RACER", "SNAKE",
                 "SOLARSYS", "SPIN", "STARFLIGHT", "TETRIS"}
        run("hmkdir", ":Games")
        for name, path in files:
            if name.upper() not in games:
                continue
            dst = ":Games:" + name
            run("hdel", dst)
            r = run("hcopy", "-r", path, dst)
            if r.returncode == 0:
                run("hattrib", "-t", "PLOS", "-c", "PLOS", dst)
                print("  hfs: copied Games/%s" % name)
    finally:
        run("humount")
    return True


def install(image, a, apps):
    with open(image, "r+b") as f:
        partno, pstart, pcount, pname = find_hfs_partition(f)
        vol = Volume(f, pstart, pcount)
        print("%s\n  partition #%d '%s' at block %d (%d blocks), volume '%s', "
              "alloc block %d bytes, %d free"
              % (image, partno, pname, pstart, pcount, vol.name, vol.al_blk_siz,
                 vol.free_bks * vol.al_blk_siz))

        kmax = a.kernel_blocks * BLK
        last = a.apps_block + RESERVE_END_OFFSET - 1
        total, already, newly = vol.reserve(a.kernel_block, last)
        print("  reserved blocks %d-%d in the volume bitmap (%d alloc blocks: %d new, %d already)"
              % (a.kernel_block, last, total, newly, already))

        loader = open(a.loader, "rb").read()
        if len(loader) > 1024:
            raise SystemExit("loader is %d bytes, boot blocks hold 1024" % len(loader))
        vol.write(0, loader.ljust(1024, b"\0"))
        print("  boot blocks: loader %d bytes" % len(loader))

        kernel = open(a.kernel, "rb").read()
        if kernel[4:8] != b"PLOS":
            raise SystemExit("kernel has no PLOS header")
        if len(kernel) > kmax:
            raise SystemExit("kernel is %d bytes, max %d (raise KERNEL_BLOCKS)" % (len(kernel), kmax))
        vol.write(a.kernel_block, kernel)
        print("  kernel: %d bytes at block %d" % (len(kernel), a.kernel_block))

        # raw app store (no-filesystem fallback): a 4-block directory + images.
        DIRBLKS = 4
        DIRMAX = (DIRBLKS * BLK - 8) // 24            # 85 entries
        if len(apps) > DIRMAX:
            raise SystemExit("raw app store holds %d programs, have %d" % (DIRMAX, len(apps)))
        dirblk = bytearray(DIRBLKS * BLK)
        dirblk[0:4] = DIR_MAGIC
        nxt = a.apps_block + DIRBLKS
        for i, (name, path) in enumerate(apps):
            data = open(path, "rb").read()
            nblocks = (len(data) + BLK - 1) // BLK
            if nxt + nblocks > a.apps_block + RESERVE_END_OFFSET:
                raise SystemExit("raw app area full")
            vol.write(nxt, data)
            struct.pack_into(">16sII", dirblk, 8 + i * 24, name.upper().encode()[:15], nxt, len(data))
            nxt += nblocks
        struct.pack_into(">I", dirblk, 4, len(apps))
        vol.write(a.apps_block, bytes(dirblk))
        print("  raw app store: %d apps (%s)" % (len(apps), ", ".join(n for n, _ in apps) or "none"))
    if a.hfs and apps:
        hfs_copy(image, partno, apps)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--image", action="append", required=True)
    ap.add_argument("--loader", required=True)
    ap.add_argument("--kernel", required=True)
    ap.add_argument("--app", action="append", default=[], help="NAME=file.pex")
    ap.add_argument("--kernel-block", type=int, default=60000)
    ap.add_argument("--kernel-blocks", type=int, default=384)
    ap.add_argument("--apps-block", type=int, default=61000)
    ap.add_argument("--hfs", action="store_true")
    a = ap.parse_args()
    if a.kernel_block + a.kernel_blocks > a.apps_block:
        raise SystemExit("kernel area overlaps the app area")
    apps = []
    for spec in a.app:
        name, path = spec.split("=", 1)
        apps.append((name, path))
    for image in a.image:
        if not os.path.exists(image):
            print("skipping missing image", image)
            continue
        try:
            install(image, a, apps)
        except PermissionError:
            raise SystemExit("%s is locked - close Mini vMac first" % image)


if __name__ == "__main__":
    main()

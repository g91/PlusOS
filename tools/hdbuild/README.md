# hdbuild - PlusOS hard-disk / floppy image builder

`hdbuild` takes the compiled PlusOS binaries, lays the volume out as an
ordinary folder, and builds that folder into a bootable disk image **from
scratch**: Driver Descriptor Record, Apple Partition Map, a freshly formatted
HFS volume (boot blocks, MDB, bitmap, both B\*-trees, files with data and
resource forks) and the raw PlusOS kernel / app-store areas.

* Pure Python 3.8+ standard library. No hfsutils, no pre-made image, no pip
  packages. Runs unchanged on Windows and Linux.
* Replaces `tools/install.py` (which needs an existing partitioned image and
  hfsutils) and `tools/make_floppy.py` (which needs hfsutils).

```
python tools/hdbuild/hdbuild.py build --out images/PlusOS.hda
```

## Quick start

| Goal | Command |
|------|---------|
| Stage + build the 64 MB hard disk | `hdbuild.py build --out images/PlusOS.hda` |
| Only lay out the folder (then edit it by hand) | `hdbuild.py stage` -> `build/disk_root/` |
| Build an image from the (edited) folder | `hdbuild.py image --root build/disk_root --out images/PlusOS.hda` |
| Bigger disk / other name | `hdbuild.py build --size 128M --volume-name "PlusOS HD" --out ...` |
| Add your own files | `hdbuild.py build --overlay my_files/ --out ...` |
| Installer floppy (replaces `make_floppy.py`) | `hdbuild.py build --preset floppy1440 --out images/PlusOS_Floppy.dsk` |
| Image that boots on real hardware | `hdbuild.py build --from-template "F:/HD60_512 PlusOS.hda" --out ...` (see [Real hardware](#real-hardware-bluescsi-and-the-scsi-driver)) |
| Check an image | `hdbuild.py verify images/PlusOS.hda` |
| Look inside | `hdbuild.py info IMAGE`, `hdbuild.py ls -lR IMAGE [path]` |
| Image back to a folder | `hdbuild.py extract IMAGE DEST` |

`build` and `image` run `verify` on the result automatically and fail if it
does not pass.

## The staged folder

`stage` creates `build/disk_root/` (floppy presets: `build/floppy_root/`). It
mirrors the Mac volume 1:1 - every file and folder you see becomes a file or
folder on the volume, so you can add, delete or replace things by hand before
running `image`.

```
build/disk_root/
    bin/ADDUSER ... bin/WHOAMI       every build/apps/*.pex, upper-cased, no extension
    Games/ASTEROIDS ...              copies of the game programs
    Desktop/Return to PlusOS         data fork (empty) of the Mac application
    multiboot.cfg                    copied byte-for-byte from the repository root
    .hdbuild.json                    the manifest (not on the volume)
    .rsrc/Desktop/Return to PlusOS.rsrc   resource fork sidecars (not on the volume)
    .boot/loader.bin                 boot blocks            (not on the volume)
    .boot/kernel.pex                 raw kernel image       (not on the volume)
    .boot/driver.bin                 SCSI driver, if any    (not on the volume)
```

Only the three top-level names `.hdbuild.json`, `.rsrc` and `.boot` are
special. Files matching the manifest's `ignore` list (`.DS_Store`,
`Thumbs.db`, ...) are skipped.

**Which programs are staged** is decided exactly like `tools/install.py`:
all `build/apps/*.pex`; if `multiboot.cfg` has `include_apps=A,B,...` only
those, plus SHELL, NETDBG and DBGRUN (always), plus any compiled program that
has no source folder under `apps/*/` ("newly discovered" in install.py);
`custom_apps=NAME:path;...` are appended. `--app NAME=file.pex` adds or
replaces one program. The games set is the same list as install.py.

**Re-running `stage` wipes the folder** (it refuses to wipe a non-empty folder
that has no `.hdbuild.json`, unless `--force`). Keep hand-made additions in a
separate folder and pass it with `--overlay DIR` (repeatable): it is copied on
top of the staged tree. In an overlay

* `Foo.bin` that is a valid MacBinary I/II/III file is decoded to `Foo`
  (type/creator/flags/dates into the manifest, resource fork to `.rsrc/`),
* an AppleDouble pair `Foo` + `._Foo` is merged the same way, AppleSingle files
  are decoded,
* an overlay-level `.hdbuild.json` is merged into the manifest (use it to give
  overlay files a type/creator),
* everything else is copied as is.

### Names

HFS names are MacRoman, 1-31 bytes (volume name: 1-27), and may not contain
`:`. Host names are converted from Unicode (NFC):

* characters MacRoman has: stored as is (`Résumé` -> `R\x8Esum\x8E`);
* characters it lacks: transliterated (accent stripped, else `_`) with a
  warning, `:` becomes `-`; with `--strict-names` this is an error instead;
* longer than 31 bytes, or empty: always an error (no silent truncation);
* two names in one folder that differ only by case or by upper/lower-case
  accents are the same name to HFS: error.

A Mac name may contain characters a host cannot store (`/`, and on Windows
`< > " \ | ? *`, trailing dot/space, `CON`, `NUL`...). In the staged folder
those bytes are written `%XX` (hex of the MacRoman byte): the Mac file `a/b`
is the host file `a%2Fb`, and a literal `%` is `%25`. `extract` produces these
escapes, `image` undoes them.

## The manifest: `.hdbuild.json`

Plain JSON, meant to be edited. Every key is optional; missing keys take the
defaults shown. Paths are relative to the staged folder (absolute paths are
allowed). Dates are `"YYYY-MM-DDTHH:MM:SS"` in Mac local time (HFS dates have
no time zone); `null` means "take it from the host file" / "now".

```jsonc
{
  "hdbuild": 1,                         // schema version
  "machine": "se",                      // informational
  "volume": {
    "name": "PlusOS",                   // <= 27 MacRoman characters
    "created": null, "modified": null,  // null = build time (or $SOURCE_DATE_EPOCH)
    "alloc_block_size": null,           // null = smallest multiple of 512 that fits
    "blessed": null,                    // folder to bless as System Folder, e.g. "System Folder";
                                        //   null = none (PlusOS boots through its boot blocks)
    "finder_info": null                 // raw 32 bytes hex (drFndrInfo); null = all zero
  },
  "image": {
    "format": "hda",                    // "hda" = DDR + partition map, "dsk" = bare HFS volume
    "size": "64M",                      // K / M / G suffix or bytes; multiple of 512
    "hfs_start": 96,                    // first block of the Apple_HFS partition (hda)
    "hfs_partition_name": "MacOS",
    "driver": null                      // or {"path": ".boot/driver.bin", "type": "Apple_Driver43",
                                        //     "name": "Macintosh", "processor": "68000",
                                        //     "bootargs": "<24 bytes hex>", "boot_size": 9392,
                                        //     "blocks": 32, "status": 127, "dd_type": 1}
  },
  "boot": { "loader": ".boot/loader.bin" },      // <= 1024 bytes; null = empty boot blocks
  "raw": {                                        // areas outside the file system; null = none
    "kernel": { "path": ".boot/kernel.pex", "block": 60000, "max_blocks": 512 },
    "apps":   { "block": 61000, "reserve_blocks": 3000, "dir_blocks": 4,
                "entries": [ {"name": "ADDUSER", "path": "bin/ADDUSER"}, ... ] }
  },
  "rules": [                            // first match wins; default type/creator of unlisted files
    {"match": "*.pex",  "type": "PLOS", "creator": "PLOS"},
    {"match": "*.cfg",  "type": "TEXT", "creator": "ttxt"},
    {"match": "*.txt",  "type": "TEXT", "creator": "ttxt"},   // also .text .ini .log .md .c .h .bas
    {"match": "bin/*",  "type": "PLOS", "creator": "PLOS"},   // extension-less programs
    {"match": "Games/*","type": "PLOS", "creator": "PLOS"},
    {"match": "*",      "type": "????", "creator": "????"}
  ],
  "ignore": [".DS_Store", "Thumbs.db", "desktop.ini", "*~", ".git", ".gitignore", "__pycache__"],
  "entries": {                          // per-path overrides; key = host path with '/'
    "Desktop/Return to PlusOS": {
      "type": "APPL", "creator": "PLOS",          // 4 chars, or "0x........" for odd bytes
      "flags": "0x2000",                          // Finder flags: number, hex string, or
                                                  //   ["hasBundle", "invisible", ...]
      "rsrc": ".rsrc/Desktop/Return to PlusOS.rsrc",
      "created": "2026-11-07T21:48:04", "modified": "2026-11-07T21:48:04", "backup": null,
      "location": [0, 0],                         // Finder icon position (v, h)
      "locked": false,
      "fxinfo": null,                             // 16 bytes hex
      "mac_name": null                            // exact Mac name if the host name cannot express it
    },
    "Games": { "created": "...", "modified": "...", "flags": "0x0000" }   // folders too
  }
}
```

* A pattern without `/` matches the file name, one with `/` the whole path;
  matching is case-insensitive.
* Resource forks are found, in this order: the entry's `rsrc`,
  `.rsrc/<path>.rsrc`, an AppleDouble file `._name` next to the file (which
  also supplies type/creator/flags unless the manifest overrides them).
* `raw.apps.entries[].path` normally points at a staged `bin/NAME`; it may
  point anywhere (e.g. `.boot/apps/NAME`) for programs that should exist only
  in the raw store.
* A folder without a manifest can be imaged too (`hdbuild.py image --root DIR`):
  you get a plain, non-bootable data volume.

## Command line

```
hdbuild.py stage   [--root DIR] [--build DIR] [--config FILE] [--machine se]
                   [--overlay DIR]... [--app NAME=FILE]... [--preset floppy1440|floppy800]
                   [--cfg-eol keep|cr|lf|crlf] [--force]
                   [--loader F] [--kernel F] [--kernel-block N] [--kernel-blocks N]
                   [--apps-block N] [--no-raw-apps] [--volume-name S] [--size S]
                   [--format hda|dsk] [--driver F]
hdbuild.py image   [--root DIR] [--out IMAGE] [--size 64M] [--volume-name S] [--format hda|dsk]
                   [--driver FILE] [--driver-type T] [--driver-bootargs HEX]
                   [--from-template IMAGE] [--hfs-start N] [--alloc-block-size N]
                   [--loader F] [--kernel F] [--kernel-block N] [--kernel-blocks N]
                   [--apps-block N] [--no-raw-apps] [--strict-names] [--no-verify]
hdbuild.py build   ...all of the above...          stage + image
hdbuild.py ls      [-l] [-R] IMAGE [path]           path: bin, bin/SHELL or :bin:SHELL
hdbuild.py info    [-v] IMAGE                       DDR, partition map, MDB, trees, raw areas
hdbuild.py extract [--force] IMAGE DEST             image -> folder + manifest
hdbuild.py verify  [--strict] [--expect-raw] IMAGE...
hdbuild.py extract-driver IMAGE OUT                 save the SCSI driver (+ OUT.json) for --driver
```

Defaults: `--build <repo>/build`, `--root build/disk_root`,
`--config <repo>/multiboot.cfg`, `--out images/PlusOS.hda`. Options given to
`image` override the manifest for that run. Exit status is 0 on success, 1 on
any error (`verify`: 1 if an image has errors; warnings fail only with
`--strict`). The output image is written to `IMAGE.tmp` and renamed, so a
failed build never leaves a half-written image behind.

`extract` is the inverse of `image`: for an image built by hdbuild,
`extract` followed by `image` reproduces the image **byte for byte** (dates,
Finder info, raw areas, driver and partition layout all travel through the
manifest). It also works on images made by hfsutils / Mac OS / Disk Studio.

`verify` is an fsck. It checks: DDR and partition map (entry counts,
overlaps, driver checksum and DDR driver table); MDB against the alternate
MDB; geometry; both B\*-trees (node map incl. map nodes, every node's type and
height, sibling links on every level, key order inside nodes and across the
leaf chain, index keys against their children, record and free-node counts);
the catalog (record sizes, a thread for every folder, parents exist, no
loops, valences, root/volume name, MDB file and folder counts, CNIDs below
`drNxtCNID`); every extent (inside the volume, no two files on one block,
physical/logical lengths); the bitmap against the extents and `drFreeBks`;
the boot blocks; and the raw areas (kernel magic, kernel inside its
reservation, reservation present in the bitmap, app directory entries inside
the reserved area, loader built for the same kernel block). Blocks that are
allocated but belong to nothing other than the PlusOS raw area are reported
as warnings.

## On-disk layout

`--format hda` (default). Block = 512 bytes.

```
disk block
0            Driver Descriptor Record  'ER', block size 512, block count, driver table
1 .. 63      Apple partition map       'PM' entries, one per block:
                                         Apple_partition_map "Apple"      1,  63 blocks
                                         Apple_Driver43      "Macintosh" 64,  32 blocks   (with a driver)
                                           -or- Apple_Free   "Extra"     64,  32 blocks   (without)
                                         Apple_HFS           "MacOS"     96,  rest of the disk
64 .. 95     SCSI driver (if supplied)  - checksum + size in its PM entry, listed in the DDR
96 ..        Apple_HFS partition  ------------------------------------------------------------
                 partition-relative block
                 0 - 1          boot blocks = boot/loader.S   ('LK', version $44xx: ROM jumps into it)
                 2              Master Directory Block ('BD')
                 3 ..           volume bitmap (1 bit per allocation block)
                 drAlBlSt ..    allocation blocks:
                                  extents-overflow B*-tree  (drNmAlBlks/128 blocks, empty)
                                  catalog B*-tree           (drNmAlBlks/128 blocks or more)
                                  file forks, contiguous, in catalog order
                 60000 ..       RAW kernel image          (KERNEL_BLOCK, max KERNEL_BLOCKS*512 bytes)
                 61000          RAW app directory 'PLDR'  (APPS_BLOCK): count + 24-byte entries
                 61004 ..       RAW app images               {char name[16]; u32 block; u32 bytes}
                 .. 63999       end of the reserved area  (APPS_BLOCK + 3000)
                 ...            free space
                 N-2            alternate MDB
                 N-1            unused
```

The HFS partition starts at block 96 whether or not a driver is present (this
is where Apple HD SC Setup puts it, and where the existing PlusOS / BlueSCSI
images have it), so `tools/install.py`'s `find_hfs_partition` and Disk Studio
find it as usual.

`--format dsk` is the same volume without blocks 0-95: boot blocks at block 0
of the file. This is what floppies and "mount an HFS volume directly"
emulators want.

The raw area (60000-63999 by default) is not a file. Its allocation blocks
are marked used in the volume bitmap and subtracted from `drFreeBks`, with the
same arithmetic as `install.py`'s `Volume.reserve` (`(block - drAlBlSt) //
(drAlBlkSiz/512)`), so neither the Mac File Manager nor hfsutils will put a
file on top of it. The block numbers are relative to the start of the HFS
partition and are compiled into the loader (`KERNEL_OFFSET`) and kernel
(`APPS_BLOCK`), see `config.mk`. Because the area ends at block 63999 **the
HFS partition must be about 31.3 MB or larger** (`--size 32M` is the practical
minimum for `hda`; the default is 64M); smaller sizes fail with a clear error.

HFS details worth knowing:

* Allocation block size is the smallest multiple of 512 that keeps
  `drNmAlBlks` <= 65535 - the same arithmetic as hfsutils' `hformat`: 512 bytes
  up to 32 MB, 1024 up to 64 MB, 2048 up to 128 MB, ...
* The catalog gets `drNmAlBlks/128` allocation blocks (more if the content
  needs it, always with at least 50 % spare nodes), so the OS can create files
  later without having to grow the tree. `drCTClpSiz`/`drXTClpSiz` are set to
  the same value, `drClpSiz` to 4 allocation blocks.
* Catalog keys are ordered by parent ID, then by name through the Mac OS
  256-entry case/diacritic folding table (the one in ROM, hfsutils and the
  Linux hfs driver - *not* ASCII order: `a` < `Z`, `_` sorts after `z`,
  `` ` `` sorts among the A's, `ä` = `Ä` != `a`).
* Folders get thread records; files do not (optional in HFS). CNIDs start at
  16 and are assigned depth-first in catalog order, so builds are
  deterministic: same folder + same manifest dates = same image bytes.
* `drFndrInfo` (blessed System Folder) stays 0: the ROM runs our boot blocks
  directly because of the `$44xx` boot-block version.
* Files larger than the biggest free run are split into at most three
  extents (only happens on floppies, around the kernel area); the
  extents-overflow tree is therefore always empty, but it is a valid tree and
  the reader/verifier fully support overflow extents in foreign images.

## How this maps to `tools/install.py`

| install.py | hdbuild |
|------------|---------|
| needs an existing partitioned `.hda` | creates DDR + partition map + HFS volume |
| `find_hfs_partition` | partition 3 `Apple_HFS` "MacOS" at block 96 (checked by the tests: install.py's own `find_hfs_partition`/`Volume` accept the image and `Volume.reserve` finds 0 new blocks to reserve) |
| `vol.write(0, loader)` | boot blocks from `.boot/loader.bin` (<= 1024 bytes) |
| `vol.write(kernel_block, kernel)`, `PLOS` check, size check | `raw.kernel` (same checks, same errors) |
| raw app store: 4-block `PLDR` directory + images, 85 entries max, area `APPS_BLOCK..+3000` | `raw.apps` (identical bytes) |
| `vol.reserve(kernel_block, apps_block+2999)` | same blocks marked in the bitmap |
| `hfs_copy`: `:bin:NAME` PLOS/PLOS | `bin/NAME` |
| `:Games:NAME` for the games set | `Games/NAME` |
| `:multiboot.cfg` TEXT/ttxt (`hcopy -r`, i.e. raw bytes) | `multiboot.cfg`, copied byte-for-byte (`--cfg-eol` to convert) |
| `:Desktop:Return to PlusOS` via `hcopy -m` | MacBinary decoded at stage time |
| `include_apps=` / `custom_apps=` | same rules, at stage time |
| `--kernel-block/--kernel-blocks/--apps-block` | same options |

`make_floppy.py` maps to `--preset floppy1440` / `floppy800`: bare `.dsk`,
volume "PlusOS Install", the essential programs only (INSTALLER SHELL DESKTOP
FILES PKG PKGMGR CAT LS SYSINFO TASKMAN CALC NOTES MEM VER BOOT REBOOT OFF
REG), `Desktop/Installer`, `Desktop/Return to PlusOS`, the installer
`multiboot.cfg`, no raw app store, kernel at block 100
(`KERNEL_OFFSET=51200`), loader `build/boot/loader_floppy.bin`. Unlike
make_floppy.py the kernel area is **reserved in the bitmap** and files are
allocated around it (make_floppy.py wrote the kernel at block 100 and then let
hfsutils copy files over the same blocks).

Note: the Makefile's floppy loader is built with `KERNEL_BYTES=184320` (360
blocks). If the kernel is larger than that, the floppy preset stops with
"kernel is N bytes, max 184320" instead of producing a floppy whose loader
would read a truncated kernel. Build the floppy loader with a bigger
`KERNEL_BYTES` and pass the same number of blocks with `--kernel-blocks`, as
the recipe below does.

## Makefile / build.bat integration

hdbuild does not edit the Makefile. Suggested replacements for the `install`
and `floppy` targets (recipe lines start with a TAB):

```make
HDBUILD := $(PY) tools/hdbuild/hdbuild.py
HDSIZE  ?= 64M
# For real hardware set one of (see "Real hardware" below):
#   HDTEMPLATE = a disk image that already boots on the machine (its driver is reused)
#   HDDRIVER   = a driver blob saved with `hdbuild.py extract-driver`
HDFLAGS := $(if $(HDTEMPLATE),--from-template "$(HDTEMPLATE)",--size $(HDSIZE)) \
           $(if $(HDDRIVER),--driver "$(HDDRIVER)",)

install: all
	$(HDBUILD) build --build $(BUILD) --machine $(MACHINE) \
	    --loader $(LOADER) --kernel $(KERNEL) \
	    --kernel-block $(KERNEL_BLOCK) --kernel-blocks $(KERNEL_BLOCKS) --apps-block $(APPS_BLOCK) \
	    $(HDFLAGS) --out "$(IMAGE_LOCAL)"

FLOPPY_KERNEL_BLOCK := 100
floppy: all
	@mkdir -p $(BUILD)/boot
	$(CC) -m68000 -DKERNEL_OFFSET="($(FLOPPY_KERNEL_BLOCK)*512)" -DKERNEL_BYTES="($(KERNEL_BLOCKS)*512)" \
	    -c boot/loader.S -o $(BUILD)/boot/loader_floppy.o
	$(OBJCOPY) -O binary $(BUILD)/boot/loader_floppy.o $(BUILD)/boot/loader_floppy.bin
	$(HDBUILD) build --preset floppy1440 --build $(BUILD) \
	    --kernel-block $(FLOPPY_KERNEL_BLOCK) --kernel-blocks $(KERNEL_BLOCKS) \
	    --out images/PlusOS_Floppy.dsk
```

`$(PY)` is whatever the Makefile already uses for Python (`python3`); the
script needs nothing but the interpreter, so `HFSUTILS=...` is no longer
required.

Two differences from the old `install` target to be aware of:

* `install.py` patched up to three *existing* images in place (local, SD card,
  test) and left every other file on them alone. hdbuild always writes a
  *fresh* volume: files PlusOS created on the old image (users, settings,
  logs) or a Mac OS System Folder on it are not carried over. To keep them,
  `hdbuild.py extract OLD.hda build/old`, copy the folders you want into an
  overlay folder and add `--overlay`.
* It builds one image. Copy it to the SD card yourself
  (`cp "$(IMAGE_LOCAL)" "$(IMAGE_REAL)"`) - and only when it was built with
  `HDTEMPLATE`/`HDDRIVER`, otherwise you replace a bootable card image with a
  driverless one.

`build.bat` needs no change if it keeps forwarding `install` /
`floppy` to make; to build an image without make/MSYS2 at all, run from the
repository root:

```bat
python tools\hdbuild\hdbuild.py build --out images\PlusOS.hda
python tools\hdbuild\hdbuild.py build --preset floppy1440 --kernel-blocks 512 --out images\PlusOS_Floppy.dsk
```

(the second line assumes `build/boot/loader_floppy.bin` was assembled with
`KERNEL_BYTES=512*512`).

## Real hardware, BlueSCSI and the SCSI driver

**An image without an `Apple_Driver` partition does not boot on a real Mac.**
When a compact Mac boots from SCSI, the ROM reads the Driver Descriptor
Record, loads the *driver from the disk*, and only that driver gives the ROM
(and our loader's `_Read` calls) access to the HFS partition. A BlueSCSI /
SCSI2SD presents the image as a raw SCSI disk, so the same applies. hdbuild
cannot ship Apple's driver, so by default it writes an `Apple_Free` partition
where the driver would be and says so loudly.

Such driverless images are fine for:

* the PlusOS web emulator (it has a paravirtual disk driver),
* stock Mini vMac-style emulators that mount an HFS volume through their own
  replacement disk driver (use `--format dsk`: they want the bare volume),
* inspection with hfsutils, Disk Studio, etc.

An emulator that emulates a real SCSI disk (the "BlueSCSI patched" Mini vMac
in `tools/emulator/` presumably does) behaves like real hardware: the ROM
needs the driver from the image. Use one of the two methods below for it.
(hdbuild's output has been checked structurally and against independent HFS
implementations; actually booting it is not part of its test suite.)

For real hardware there are two ways to get the driver in:

1. **`--from-template IMAGE`** (recommended). Give it any image that already
   boots on your hardware (e.g. the BlueSCSI image you have been using, or a
   blank image initialised with Apple HD SC Setup / a patched one). hdbuild
   copies everything in front of that image's `Apple_HFS` partition - DDR,
   partition map, driver partition - verbatim, and builds a brand-new HFS
   volume into the partition. The image size is the template's; add `--size`
   to grow/shrink it (allowed when the HFS partition is the last one; the DDR
   block count and the HFS map entry are patched).

   ```
   hdbuild.py build --from-template "F:/HD60_512 PlusOS.hda" --out images/PlusOS.hda
   ```

2. **`--driver FILE`**. Embed a driver blob you extracted yourself:

   ```
   hdbuild.py extract-driver "F:/HD60_512 PlusOS.hda" build/scsi_driver.bin
   hdbuild.py build --driver build/scsi_driver.bin --size 64M --out images/PlusOS.hda
   ```

   `extract-driver` also writes `build/scsi_driver.bin.json` (partition type,
   name, processor, boot size, boot arguments, DDR type); `--driver` picks it
   up. hdbuild then writes an `Apple_Driver43` partition at block 64 (32
   blocks), fills in `pmBootSize`, the ROM's driver checksum `pmBootCksum`,
   `pmProcessor` "68000" and the boot-argument bytes, and adds the DDR driver
   entry (`ddBlock`, `ddSize` in blocks, `ddType` 1). Without the `.json` the
   whole file is taken as the driver and Apple's usual `Apple_Driver43` values
   are used (`--driver-type`, `--driver-bootargs` override). The driver is
   copied into the staged folder (`.boot/driver.bin`) and recorded in the
   manifest.

Writing the image to a BlueSCSI SD card: the image is a plain raw disk, so
just copy it to the FAT32/exFAT card under a BlueSCSI name, e.g.
`HD60_512 PlusOS.hda` (SCSI ID 6, LUN 0, 512-byte blocks):

```
copy images\PlusOS.hda "F:\HD60_512 PlusOS.hda"
```

or build straight onto the card with `--out "F:/HD60_512 PlusOS.hda"`.
(`dd`/Etcher are *not* used: the card keeps its FAT file system and the
BlueSCSI firmware serves the file.)

Emulators: `tools/vmac.ps1 start -Image images\PlusOS.hda` (the script
already looks for `images\PlusOS.hda`); for the floppy use
`images\PlusOS_Floppy.dsk`.

## Limits

* Volume size: 800 K ... just under 2 GB (the limit of the Plus/SE-era File
  Manager). 65535 allocation blocks at most, so the allocation block grows
  with the volume (32 K blocks at 2 GB).
* `hda` images with the default raw layout: >= 32 MB.
* Names: MacRoman, 31 bytes (volume 27), no `:`.
* Forks: < 2 GB each; at most 3 extents per fork (a from-scratch build never
  needs more; if the free space is too fragmented by raw areas you get a
  "volume full" error rather than an overflow extent).
* Raw app store: 85 programs, 3000 blocks including the 4-block directory
  (same as install.py); `--no-raw-apps` drops it (the kernel then loads
  programs through the File Manager only).
* One HFS partition per image. Extra partitions survive only via
  `--from-template`.
* File thread records, Desktop DB and Finder comments are not written.

## Tests

```
cd tools/hdbuild
python -m unittest                    # everything (oracle tests skip if unavailable)
python -m unittest tests.test_btree   # one module
```

| Module | What it covers |
|--------|----------------|
| `tests/test_names.py` | the ordering table against known Mac sort results, MacRoman encoding/transliteration, host-name escaping for all 255 byte values |
| `tests/test_btree.py` | geometry vs hformat/known images, empty volume, 3000 files (4-level catalog), 11000 files (map nodes), 60-deep nesting, split extents around a raw area, exact fill, volume full, duplicate/illegal names, and that `verify` really detects nine kinds of damage |
| `tests/test_roundtrip.py` | staging rules (filter, custom apps, overlay, MacBinary/AppleDouble), raw areas byte-for-byte, **install.py's own parser on the image**, stage -> image -> extract -> image identical (hda 512/1024-byte blocks, dsk, with driver), `--driver`, `--from-template`, floppy presets, manifest edits, CLI |
| `tests/test_oracle.py` | optional independent readers: **hfsutils** (native, or in WSL - distro `$HDBUILD_WSL_DISTRO`, default `Ubuntu`): `hls -lR`, `hcopy -r`/`-m` of every file incl. one-character names for every MacRoman byte, and hfsutils *writing* 150 files into our volume followed by `verify`; the ordering table against the one inside the hfsutils binary; **machfs** (pip, if importable); the repo's C++ reader **PlusOSDiskStudio_linux** (`info`, `ls`) |

## Source map

```
tools/hdbuild/hdbuild.py        CLI entry point
tools/hdbuild/hdlib/macroman.py MacRoman names, catalog ordering table, %XX host names
tools/hdbuild/hdlib/hfs.py      on-disk structures, B*-tree builder, VolumeBuilder (formatter/writer)
tools/hdbuild/hdlib/hfsread.py  volume reader (catalog, overflow extents, forks)
tools/hdbuild/hdlib/fsck.py     verify
tools/hdbuild/hdlib/apm.py      DDR + partition map, driver checksum
tools/hdbuild/hdlib/forks.py    MacBinary / AppleSingle / AppleDouble
tools/hdbuild/hdlib/manifest.py .hdbuild.json schema, defaults, dates, sizes, rules
tools/hdbuild/hdlib/stage.py    stage
tools/hdbuild/hdlib/image.py    image, extract, raw-area detection
tools/hdbuild/hdlib/cli.py      argument parsing and the commands
```

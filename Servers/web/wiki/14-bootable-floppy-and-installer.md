# Minimal Bootable Floppy & Hard Drive Installer

**PlusOS** provides a dedicated, minimal bootable floppy disk image (`images/PlusOS_Floppy.dsk`) designed for machine recovery, initial hardware provisioning, and minimal footprint boots on vintage Macintosh computers.

---

## Architecture & Design Goals

Unlike the full hard drive distribution (`PlusOS.hda`), the bootable floppy disk is built with strict size constraints and zero bloat:

- **Minimal Footprint**: Total image size is ~500 KB, leaving ample room on both **800K DSDD** and **1.44 MB High-Density (SuperDrive)** floppy disks.
- **Strictly Zero Games**: Arcade games and emulator packages are completely excluded from the floppy distribution.
- **Dedicated Floppy Bootloader**: Uses `boot/loader_floppy.bin` optimized for floppy drive step timings and sector interleaves.
- **Includes Guided Hard Drive Installer (`installer.pex`)**: Automatically probes all SCSI targets (`0`–`7`), formats unpartitioned or existing drives as Macintosh HFS, writes boot blocks, and installs the operating system.

```text
+------------------------------------------------------------------------+
|               PlusOS Bootable Floppy Disk Structure (.dsk)              |
+------------------------------------------------------------------------+
|                                                                        |
|  [Sector 0-1]  HFS Boot Blocks (`boot/loader_floppy.bin`)              |
|                - Checks hardware, sets stack below BufPtr              |
|                                                                        |
|  [Block 100]   PlusOS Kernel (`build/kernel/kernel.pex`)               |
|                - Preemptive scheduler, memory manager, trap table      |
|                                                                        |
|  [HFS Volume]  "PlusOS Install"                                        |
|                |                                                       |
|                +-- :bin:                                               |
|                |   - Essential tools ONLY:                             |
|                |     INSTALLER, SHELL, DESKTOP, FILES, PKG, PKGMGR,    |
|                |     CAT, LS, SYSINFO, TASKMAN, CALC, NOTES, MEM,      |
|                |     VER, BOOT, REBOOT, OFF, REG                       |
|                |                                                       |
|                +-- :Desktop:                                           |
|                |   - Installer (Desktop shortcut to installer.pex)     |
|                |   - Return to PlusOS (APPL / PLOS for Classic Mac)    |
|                |                                                       |
|                +-- :multiboot.cfg                                      |
|                    - Default boot target: PlusOS Installer             |
|                                                                        |
+------------------------------------------------------------------------+
```

---

## Hardware Capacity Rules: 800K vs 1.44M

Vintage Macintosh systems utilized two primary 3.5-inch floppy formats:

| Format | Raw Capacity | Usable HFS Space | Compatible Drives / Models |
|---|---|---|---|
| **800K DSDD** (Double-Sided) | 819,200 bytes | ~780 KB | Macintosh Plus, Macintosh SE (800K Sony drive) |
| **1.44M HD** (High Density) | 1,474,560 bytes | ~1.40 MB | Macintosh SE FDHD, SE/30, SuperDrive |

> [!IMPORTANT]
> **Base OS Core Calculation & Floppy Limits:**
> Every bootable disk requires the core operating system to boot:
> - Boot Blocks (`loader.bin`): 1 KB
> - PlusOS Kernel (`kernel.pex`): ~165 KB
> - HFS Volume Structures & Config: ~25 KB
> - **Total Base Boot Core**: **~191 KB** (Required on every disk).
>
> When selecting software packages in **PlusOS Disk Studio**:
> - For **800K Floppy Disks** (Mac Plus & standard Mac SE 800K Sony drives): Total bootable size must stay under **800 KB**, leaving **~609 KB** maximum for user applications and utilities.
> - For **1.44M HD Disks** (Macintosh SE FDHD / SuperDrive): Total bootable size supports up to **1.44 MB**, leaving **~1.25 MB** for packages.

PlusOS Disk Studio displays these limits dynamically:
- When package size is under 800 KB: Status shows `Fits 800K & 1.44M Disks`.
- When package size is between 800 KB and 1.44 MB: Status warns `EXCEEDS 800K! Requires 1.44M HD`.
- When package size exceeds 1.44 MB: Status warns `EXCEEDS 1.44M! (Hard Drive Only)`.

---

## Building the Floppy Image

### Method 1: One-Click GUI in PlusOS Disk Studio
1. Launch `PlusOSDiskStudio.exe` from the repository root.
2. Under Section 4 (**Build Execution & Disk Manager**), click **"Build Bootable Floppy (.dsk)"**.
3. Disk Studio runs the build asynchronously in a worker thread, displays live compilation output, and generates `images/PlusOS_Floppy.dsk`.
4. When finished, Disk Studio prompts you to inspect the floppy directly in the HFS Explorer.

### Method 2: Command Line (Linux / WSL / macOS)
```bash
# Build standard 1.44M installer floppy
make floppy

# Or invoke the Python tool directly with custom size and label:
python3 tools/make_floppy.py --size 800k --output images/PlusOS_Floppy_800K.dsk
python3 tools/make_floppy.py --size 1.44m --output images/PlusOS_Floppy_1.44M.dsk
```

### Method 3: Command Line (Windows)
```cmd
build.bat floppy
```

---

## Guided Hard Drive Installation (`installer.pex`)

When you boot your Macintosh from the PlusOS floppy, the installer launches automatically:

1. **SCSI Hardware Probe**:
   The installer queries SCSI IDs `0` through `7`, identifying connected hard drives (Apple 20SC/40SC, BlueSCSI, RaSCSI, SCSI2SD).
2. **Drive Selection & Confirmation**:
   Select your desired target SCSI target ID.
3. **Partitioning & Formatting**:
   The drive is formatted as a Macintosh HFS volume named `PlusOS HD`.
4. **Writing Boot Blocks & Kernel**:
   `boot/loader.bin` is written to sectors 0–1; the kernel image is installed into reserved contiguous blocks.
5. **Seeding Applications & Theming**:
   System binaries, desktop utilities, games, and `multiboot.cfg` are copied onto the volume.
6. **Classic Mac OS Integration**:
   The Classic Mac OS application `Return to PlusOS` (`APPL` / `PLOS`) is written to `:Desktop:Return to PlusOS`, allowing seamless dual-boot switching.

---

## Testing on Mini vMac

You can verify the floppy disk image immediately in the Mini vMac emulator:
1. Drag `images/PlusOS_Floppy.dsk` into the Mini vMac window.
2. The floppy boots instantly into the guided installer or recovery desktop.
3. Drag a blank `.dsk` hard drive image (e.g. 32 MB) into Mini vMac to test end-to-end installation.

---

## Navigation

- **Previous Chapter**: [← 13. PlusOS Disk Studio & Compiler Pipeline](/wiki/13-plusos-disk-studio-and-compiler)
- **Next Chapter**: [15. Package Repository Server & Software Center →](/wiki/15-package-repository-and-network-management)
- **Table of Contents**: [Return to Getting Started](/wiki/01-getting-started)

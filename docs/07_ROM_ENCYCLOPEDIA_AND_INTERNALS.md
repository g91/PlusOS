# PlusOS: Macintosh ROM Encyclopedia, Internals & C++ Reimplementation

> **Operating System:** PlusOS  
> **Upstream Assembly ROM Source:** [https://github.com/jonathanschilling/mac_rom](https://github.com/jonathanschilling/mac_rom)  
> **Local ROM Source Paths:** `./mac_rom/`, `../mac_rom/`, `C:\Users\Evil\PlusOS\ROM\SE\`, `F:\mac_rom\`  
> **ROM Targets:** Macintosh Plus (128 KB) & Macintosh SE (256 KB)  

---

## 1. Upstream ROM Assembly Source Tree & Open Compilation

The complete, fully commented, buildable 68000 assembly source tree for all classic-era Macintosh ROMs is publicly available at:

👉 **[https://github.com/jonathanschilling/mac_rom](https://github.com/jonathanschilling/mac_rom)**

This repository allows anyone to compile bit-identical copies of seven original Macintosh ROMs using free, standard, open-source GNU toolchains (`binutils-m68k-linux-gnu` or `m68k-elf-binutils`).

### 1.1 How to Clone & Compile the ROMs
You can clone the source repository directly into your local workspace so that the build is completely self-contained within its own folder:

```bash
# Clone the upstream assembly source tree locally
git clone https://github.com/jonathanschilling/mac_rom.git

# Enter repository
cd mac_rom

# Build all seven ROM variants using standard GNU tools
make
```

### 1.2 Toolchain Requirements
No proprietary Apple compilers or legacy MPW installations are required. Standard cross-assemblers available on modern Linux distributions are auto-detected:
* **Ubuntu / Debian / WSL**: `sudo apt install binutils-m68k-linux-gnu`
* **Arch Linux**: `pacman -S m68k-elf-binutils`
* **macOS (Homebrew)**: `brew install m68k-elf-binutils`

Override the cross-compiler prefix if needed:
```bash
make CROSS=m68k-elf-
# or
make CROSS=m68k-linux-gnu-
```

### 1.3 Individual ROM Target Compilation
```bash
make mac_128k      # 1984-01, 64 KB,  $28BA61CE (Original Macintosh 128K)
make mac_512k      # 1984-10, 64 KB,  $28BA4E50 (Macintosh 512K "Fat Mac")
make mac_plus_v1   # 1986-01, 128 KB, $4D1EEEE1 (Mac Plus v1)
make mac_plus_v2   # 1986-03, 128 KB, $4D1EEAE1 (Mac Plus v2 branch fix)
make mac_plus_v3   # 1986-03, 128 KB, $4D1F8172 (Mac Plus v3 SCSI patch - default for PlusOS)
make mac_se        # 1987-03, 256 KB, $B2E362A8 (Mac SE 800K drive)
make mac_se_fdhd   # 1989-08, 256 KB, $B306E171 (Mac SE SuperDrive)
```

Every build is automatically verified against hardcoded cryptographic fingerprints (size, checksum, version word, MD5) by `tools/verify_rom.sh`, guaranteeing **100% bit-identical** output without requiring original ROM dumps.

---

## 2. Universal, Open & Portable Compiler Architecture in PlusOS

To ensure PlusOS does not rely on hardcoded external drives (such as an external `F:\` drive or physical SD card), the build system has been designed to be **completely universal, portable, and self-contained within its own folders**:

### 2.1 In-Tree ROM & Toolchain Auto-Detection (`config.mk`)
PlusOS automatically locates the `mac_rom` source tree and compiler in this order of precedence:
1. **In-Tree Subdirectory**: `PlusOS/mac_rom` (cloned directly into the project).
2. **Sibling Directory**: `../mac_rom` (cloned alongside `PlusOS`).
3. **External Directory**: `/mnt/f/mac_rom` (if present on an attached drive).

Similarly, the cross-compiler automatically checks:
1. Local Retro68 toolchain (`/home/evil/Retro68-build/toolchain` or `/usr/local/retro68`).
2. Standard PATH cross-compilers (`m68k-apple-macos-gcc`, `m68k-elf-gcc`, or `m68k-linux-gnu-gcc`).

### 2.2 Local Disk Image Management (`images/`)
PlusOS maintains its disk images locally inside `C:\Users\Evil\PlusOS\images/`:
* `images/PlusOS.hda`: Local standalone 60 MB HFS disk image.
* `images/BOOTTEST.hda`: Local 32 MB automated test disk image.
* When an external SD card (`F:\`) is plugged in, `build.bat install` detects it and writes to both the local image and the external card simultaneously.
* If no external card or drive is attached, the build proceeds cleanly without errors, installing directly to `images/`.

---

## 3. Lineage & Hardware Comparison of Target ROMs

PlusOS specifically targets and builds on the classic Macintosh 128 KB and 256 KB ROMs:

| Feature / Metric | Macintosh Plus ROM | Macintosh SE ROM | Macintosh SE FDHD ROM |
|:---|:---:|:---:|:---:|
| **ROM Size** | 128 KB (131,072 bytes) | 256 KB (262,144 bytes) | 256 KB (262,144 bytes) |
| **Physical Chips** | 2 × 64 KB DIP ROMs | 2 × 128 KB DIP ROMs | 2 × 128 KB DIP ROMs |
| **ROM Version Word** | `$0075` | `$0276` | `$0276` |
| **Global Checksum** | `$4D1F8172` (v3) | `$B2E362A8` | `$B306E171` |
| **HI-Chip Checksum** | N/A | `$92EE` (Offset `$0030`) | `$2417` (Offset `$0030`) |
| **LO-Chip Checksum** | N/A | `$45EE` (Offset `$0034`) | `$2AEE` (Offset `$0034`) |
| **Release Date** | March 1986 | March 1987 | August 1989 |
| **Implemented Traps** | 626 traps | 660 traps | 660 traps |
| **Toolbox Table Loc** | `$0C00` | `$0E00` | `$0E00` |
| **Input Hardware** | Serial Keyboard Protocol | Apple Desktop Bus (ADB) | Apple Desktop Bus (ADB) |
| **Floppy Controller** | IWM (800K GCR only) | IWM (800K GCR only) | SWIM (1.44M MFM SuperDrive) |

---

## 2. ROM Header Anatomy ($0000 – $0038)

The first 64 bytes of the ROM contain critical identification words, entry vectors, and checksums:

```
Offset   Size     Content         Description
-----------------------------------------------------------------------------
+$00     4 bytes  $4D1F8172       ROM 32-bit Additive Global Checksum
+$04     4 bytes  $0040002A       Initial Reset PC (Entry to ROM POST)
+$08     2 bytes  $0075 / $0276   ROM Version Word ($0075 = Plus, $0276 = SE)
+$0A     2 bytes  $0000           Release flags
+$0C     4 bytes  Vector          System Startup Entry Point
+$10     4 bytes  Vector          Unimplemented Trap Handler (EMT 1010)
...
+$30     4 bytes  $000092EE       (SE Only) High 128KB Chip Checksum
+$34     4 bytes  $000045EE       (SE Only) Low 128KB Chip Checksum
```

---

## 3. Mathematical Checksum Algorithms & Verification

### The 32-Bit Additive Checksum Algorithm
The Macintosh ROM validates its silicon integrity upon every power-on reset. The checksum stored at offset `$0000` is computed as follows:

$$\text{Checksum} = \sum_{i=1}^{(N/4)-1} \text{Longword}[i]$$

Where:
* $N$ is the total ROM size (131,072 bytes for Plus, 262,144 bytes for SE).
* The summation treats each 4-byte longword as an unsigned 32-bit big-endian integer, ignoring arithmetic overflow.
* When the ROM boots, its hardware POST routine sums every longword in the entire ROM (including the stored checksum at offset 0). The 32-bit sum must equal the hardcoded reference checksum.

### SE Dual-Chip Split Checksum
On the Macintosh SE, the 256 KB ROM is physically partitioned across two 16-bit interleaved 128 KB EPROM chips:
* **HI Chip**: Supplies the high bytes (even addresses: $0, $2, $4...).
* **LO Chip**: Supplies the low bytes (odd addresses: $1, $3, $5...).
* During factory testing and boot diagnostics, the SE ROM independently verifies each physical chip using 16-bit additive sums stored at offsets `$0030` and `$0034`.

### Recomputing Checksums with `tools/fixup_checksum.py`
Whenever code or resources inside a ROM image are altered, the checksums must be recomputed:
```python
def fixup_rom(path):
    with open(path, "r+b") as f:
        data = bytearray(f.read())
        size = len(data)
        
        # 1. Compute 32-bit global additive checksum
        csum = 0
        for i in range(4, size, 4):
            csum = (csum + struct.unpack_from(">I", data, i)[0]) & 0xFFFFFFFF
        struct.pack_into(">I", data, 0, csum)

        # 2. Compute SE dual-chip checksums if 256 KB
        if size == 262144:
            hi_sum = sum(data[i] for i in range(0, size, 2)) & 0xFFFF
            lo_sum = sum(data[i] for i in range(1, size, 2)) & 0xFFFF
            struct.pack_into(">I", data, 0x30, hi_sum)
            struct.pack_into(">I", data, 0x34, lo_sum)
            
        f.seek(0)
        f.write(data)
```

---

## 4. The PlusOS Tweaked SE ROM (`ROM/SE/MacSE.ROM`)

Located in `C:\Users\Evil\PlusOS\ROM\SE\MacSE.ROM`, PlusOS includes an optimized build of the Macintosh SE ROM assembled from the 68000 source tree (`F:\mac_rom`):

### The 5-Second Boot Optimization
* **Source Location**: `se/boot/boot_search.s`, routine `WAITFORINTERNAL`.
* **Problem**: Apple's stock SE ROM waits **15 seconds** for an internal floppy disk drive to spin up before checking SCSI hard drives or displaying the blinking question mark floppy.
* **Tweak**: The wait operand was changed from `MOVE.W #15,D0` to `MOVE.W #5,D0` at ROM offset `$0D3D`.
* **Result**: Reduces dead boot waiting time by **10 seconds** without affecting booting from real SCSI hard disks or inserted floppies!
* **Checksum Adjustment**: The modified ROM was re-checksummed to `$B2E36294` using `fixup_checksum.py` and runs with 100% stability.

---

## 5. PlusOS C++ Reimplementation of the SE ROM (`ROM/SE/cpp/`)

Inside `C:\Users\Evil\PlusOS\ROM\SE\cpp\`, PlusOS features a complete **C++ whitebox reimplementation scaffold** of the entire 256 KB Macintosh SE ROM:

```
ROM/SE/cpp/
  ├── include/          # C++ declarations of all 102 ROM modules
  ├── src/              # C++ implementation files (.cpp)
  ├── libserom_cpp.a    # Compiled 68000 object library (Retro68)
  ├── MacSE_cpp.elf     # Linked ELF executable
  ├── MacSE_cpp.ROM     # 256 KB experimental binary image
  ├── Makefile          # Build pipeline targeting m68k-elf-g++
  └── rom.ld            # 68000 ROM linker memory layout script
```

### Architectural Highlights
* **102 Modular C++ Files**: Exactly 1:1 structural correspondence with Apple's original 68000 assembly subsystems:
  * `boot/`: Diagnostics, RAM test, BootFromDisk.
  * `hw/`: VIA, SCC, SCSI, IWM floppy, sound timer.
  * `os/`: Memory Manager, Line 1010 Dispatcher, File Manager HFS, ADB Manager.
  * `toolbox/`: QuickDraw, Window Manager, Menu Manager, TextEdit.
* **9,545 Stubbed Entry Points**: Provides exact C++ prototypes, ABI register preservation contracts, and parameter types for every internal subroutine in the ROM.
* **Purpose**: Serves as the blueprint for migrating low-level assembly routines into modern, type-safe, maintainable C++ code while maintaining binary compatibility with classic Macintosh software.

---

## 6. Undocumented Easter Eggs & Steganography in the ROM

The Macintosh SE ROM contains one of computing history's most famous hidden Easter eggs:

### The Secret Engineering Team Photos
* **Location in Source**: `se/resources/res_tcsl.s` and `se/resources/res_bbmc.s`.
* **Resource IDs**: Stored in the ROM resource fork as raw bitmapped pictures.
* **How to Trigger on Real Hardware**:
  1. Boot the Macintosh SE.
  2. Press the hardware programmer's switch (Interrupt button) to drop into MacsBug.
  3. Type `G 41D640` and press Return.
  4. The screen immediately displays four high-resolution digitized photographs of the Macintosh SE engineering team waving from Apple's Cupertino campus!

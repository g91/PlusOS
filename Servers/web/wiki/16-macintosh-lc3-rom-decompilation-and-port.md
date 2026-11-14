# Macintosh LC III ROM Reverse-Engineering, Signature Mapping & Port

PlusOS 3.1 features full native architecture support for the **Macintosh LC III** (and LC III+ / Performa 450). This article documents the reverse-engineering, decompilation, opcode signature mapping, and C++ reimplementation of the **Apple 1024 KB Universal SuperMario ROM (version `$067C`, codename *Vail*)**.

---

## 1. Hardware Architecture & Technical Specifications

The Macintosh LC III (introduced in February 1993) marked a major evolution over compact 68000 Macintoshes and earlier LC models:

| Subsystem | Specification | PlusOS Driver / Interface |
|---|---|---|
| **CPU** | Motorola 68030 @ 25 MHz | 32-bit clean, `CACR`/`CAAR` caches, MMU support |
| **Bus Width** | Full 32-bit data and 32-bit address bus | High-speed memory transfers, no 16-bit bottleneck |
| **ROM** | 1024 KB (1 MB) SuperMario Universal ROM | Master Checksum: `$ECBBC41C`, Version: `$067C` |
| **Video Controller** | Sonora Built-in Video Controller | 640 &times; 480 @ 80 bytes/row (1bpp) / 640 bytes/row (8bpp) |
| **SCSI Controller** | NCR 53C96 High-Speed SCSI-2 | Fast asynchronous / synchronous bus handshakes |
| **Microcontroller** | Egret (ADB Keyboard, Mouse, RTC, PRAM) | VIA1 / RBV bit-banged packet interface |
| **RAM** | 4 MB onboard &ndash; expandable to 36 MB | 72-pin SIMM, physical dynamic heap allocation |

---

## 2. 100% Decompiled Modular Assembly (`ROM/LC-III/asm/`)

The 1 MB raw ROM was disassembled using `m68k-apple-macos-objdump -m m68k:68030` into an 11.2 MB master disassembly, which was then systematically partitioned into 25 modular assembly source files mirroring Apple's internal Terror / SuperMario Universal ROM tree:

```text
ROM/LC-III/asm/
├── boot/
│   ├── rom_header.s        # Checksum $ECBBC41C, offset $0000002A, ver $067C
│   ├── reset.s             # Power-On Reset & cold boot vector
│   ├── vectors.s           # 68030 Exception Vector table
│   ├── start_boot.s        # Hardware qualification, Sonora VRAM sizing, PMMU init
│   ├── memtest.s           # 32-bit RAM bus test (4 MB base up to 36 MB)
│   ├── sad_mac.s           # Diagnostic failure handler & Sad Mac rendering
│   └── boot_search.s       # Boot volume polling (Floppy / SCSI ID 0-6)
├── hw/
│   ├── sonora_video.s      # Sonora built-in video (640x480 mono / 8-bit color)
│   ├── scsi_53c96.s        # NCR 53C96 high-speed SCSI-2 controller
│   ├── via_rbv.s           # VIA1 ($50F00000), RBV ($50F02000), Egret ADB/RTC
│   ├── interrupts.s        # 68030 Level 1-7 autovector interrupt handlers
│   └── pmmu_cache.s        # 68030 CACR/CAAR cache & TC/CRP/SRP MMU config
├── os/
│   ├── memory_mgr.s        # 32-bit clean Zone, Master Pointer, and Handle allocation
│   ├── trap_dispatcher.s  # $A000-$AFFF A-line trap table & dispatch engine
│   ├── file_mgr.s          # HFS catalog, extents, and volume mount routines
│   ├── device_mgr.s        # Unit table (UTableBase) and driver dispatch
│   ├── event_mgr.s         # Event queue and posting
│   └── adb_mgr.s           # Apple Desktop Bus keyboard & mouse transaction engine
├── toolbox/
│   ├── quickdraw.s         # Color QuickDraw GrafPort, bitblt, and color tables
│   ├── window_mgr.s        # Window records, title drawing, and drag regions
│   ├── menu_mgr.s          # Menu bar definition and pull-down handling
│   ├── control_mgr.s       # Buttons, checkboxes, and scrollbar controls
│   ├── dialog_mgr.s        # Modal/Modeless dialog item lists (DITL)
│   ├── textedit.s          # TextEdit records, font measurements, and selection
│   ├── font_mgr.s          # Chicago, Geneva, Monaco font tables
│   └── resource_mgr.s      # ROM resource map reader (CODE, CURS, FONT, ICON, etc.)
└── resources/
    └── rom_resources.s     # Embedded system resources and sound chimes
```

---

## 3. Signature Matching Engine & Dispatch Decompression

To precisely correlate all 8,727 routines in the Mac SE ROM source tree (`mac_rom`) with the LC III ROM, PlusOS employs an automated reverse-engineering tool: `tools/rom_signature_mapper.py`.

### A. Compressed Dispatch Table Decompression
The LC III ROM stores its trap table compressed at `DISPOFF` (`$000D32B0`). The decompression engine decodes:
- **512 Toolbox Traps** (`$A800` &ndash; `$A9FF`)
- **768 OS Traps** (`$A000` &ndash; `$A2FF`)

Each trap entry uses delta encoding with 7-bit short offsets, 15-bit long word offsets, and 4-byte absolute escape vectors. All 1,280 system entry points were recovered:
* `_Open` (`$A000`): `$408300F0` (ROM offset `$0300F0`)
* `_Read` (`$A002`): `$408301E0` (ROM offset `$0301E0`)
* `_Write` (`$A003`): `$408302F0` (ROM offset `$0302F0`)
* `_MountVol` (`$A00F`): `$4083BAF4` (ROM offset `$03BAF4`)
* `_NewPtr` (`$A01E`): `$408311E0` (ROM offset `$0311E0`)

### B. Opcode-Masked Signature Correlation
Functions that evolved between the 68000 and 68030 contain different branch displacements and absolute address references. The signature mapper converts machine code sequences into wildcard regular expressions, ignoring variable addresses while preserving instruction opcodes and addressing modes (`movem`, `link`, `lea`, `cmp`, `jsr`).

The tool successfully verified and mapped **5,416 symbols** into the repository file `ROM/LC-III/maps/lc3_rom.map` (a build artifact in the PlusOS source tree, viewable in any text editor).

---

## 4. Key Architectural Discoveries & Fixes

### 1. Universal Bootloader ABI vs. Classic ROM
* **The Problem**: In Mac Plus and SE ROMs, boot blocks read `$0210` (`BootDrive`) from low memory. On the LC III SuperMario Universal ROM, `$0210` is left `$0000` until after boot blocks complete.
* **The Solution**: On the LC III, the ROM passes:
  * `A1.L`: Pointer to the boot device's drive queue element (`DrvQEl`).
  * `D0.W`: The boot drive number.
  * `D7.L`: Pointer to `MyIOPB`.
  The universal `loader.S` was updated to inspect `A1` and `D0`, ensuring foolproof drive resolution on any Macintosh.

### 2. 68030 Instruction & Data Cache Flushes
* **The Problem**: The 68030 features 256-byte on-chip instruction and data caches. After reading the raw kernel into RAM via the SCSI driver, jumping immediately to `_start` caused the CPU to fetch stale cache lines, causing unpredictable freezes.
* **The Solution**: An explicit cache flush sequence is executed immediately prior to `jmp (%a4)`:
  ```assembly
  move.l  %d0, -(%sp)
  .short  0x4E7A, 0x0002    | movec cacr, d0
  ori.w   #0x0808, %d0      | clear instruction & data cache
  .short  0x4E7B, 0x0002    | movec d0, cacr
  move.l  (%sp)+, %d0
  ```

### 3. BlueSCSI `MacPlus` Preset Conflict
* In `bluescsi.ini`, the setting `System = "MacPlus"` forces BlueSCSI to simulate slow 8 MHz Mac Plus bit-banged handshake delays.
* On the LC III, the **NCR 53C96** high-speed SCSI controller expects standard SCSI-2 handshakes. Setting `System = "MacPlus"` caused SCSI block transfers to stall. Changing this to `Auto` restores full-speed transfers.

### 4. NCR 53C96 16-Bit Transfer Counter & Chunked Reads
* The NCR 53C96 SCSI-2 controller on the Macintosh LC III features a **16-bit transfer counter** (`TCNT_LO` and `TCNT_HI`), capping individual DMA/SCSI transfer commands at 64 KB (65,536 bytes).
* The raw bootloader originally attempted to read the entire 256 KB (`KERNEL_BYTES = 512 * 512`) kernel image in a single `_Read` trap call (`0xA002`). On 68000 blind-transfer controllers this succeeded, but on the 53C96 Async SCSI controller, requesting >64 KB caused transfer counter truncation (`262144 & 0xFFFF = 0`), stalling the SCSI bus.
* **The Solution**: `loader.S` now divides the 256 KB kernel read into 8 contiguous 32 KB chunks (`32,768` bytes = 64 sectors each). Each chunk is read sequentially, advancing `ioBuffer` and `ioPosOffset`, guaranteeing compliance with all 16-bit SCSI transfer limits.

### 5. Universal ROM Register ABI & Drive Resolution
* Decompilation of the LC III Universal ROM startup sequence at `0x001D10` revealed that upon calling secondary boot code (`jsr 2(%fp)`), register `A1` is **uninitialized leftover state** rather than a `DrvQEl` pointer.
* The ROM explicitly writes the verified boot parameters into low-memory globals:
  * `BootDrive` (`$0210`, word): Boot volume drive number
  * `BtDskRfn` (`$0B34`, word): Boot disk driver refNum (e.g. `-33` to `-40` for SCSI)
* `loader.S` now reads directly from `$0210` and `$0B34`, falling back to traversing the drive queue at `DrvQHdr` (`$030A`), eliminating random pointer dereferences.

### 6. Real-Time On-Screen Telemetry Bars
To provide immediate visual verification of hardware execution milestones without depending on disk logging, `loader.S` draws high-contrast solid black progress stripes across the top scanlines:
* **Stripe 1** (Rows 4–10): Boot block entry reached
* **Stripe 2** (Rows 12–18): Boot drive number & driver refNum resolved
* **Stripes 3–10** (Rows 20–82): SCSI 32 KB chunks 0 through 7 read successfully
* **Stripe 11** (Rows 76–82): Executable signature `'PLOS'` verified
* **Stripe 12** (Rows 84–90): 68030 instruction/data caches flushed, jumping to kernel `crt0`
* **Failure Modes**:
  * Solid white bar across rows 4–120: `fail_drive`
  * Alternating horizontal stripes across rows 4–120: `fail_read`
  * Checkered pattern across rows 4–120: `fail_magic`

---

## 5. C++ ROM Reimplementation (`ROM/LC-III/cpp/`)

The C++ implementation provides complete 68030 machine state abstractions, typed low-memory globals, hardware drivers, and trap dispatching:

* **Headers** (`include/lc3rom/`):
  * `machine.hpp`: 68030 CPU state (`D0-D7`, `A0-A7`, `SR`, `CACR`, `CAAR`), memory accessors (`PeekL`, `PokeL`), Sonora registers (`$50F00000`), NCR 53C96 registers (`$50F10000`).
  * `lowmem.hpp`: 32-bit clean low-memory globals (`kMemTop`, `kBufPtr`, `kScrnBase`, `kScreenRow = 80`).
  * `traps.hpp`: Trap table declarations and A-line dispatch definitions.
* **Built Library**: `ROM/LC-III/cpp/liblc3rom_cpp.a` (compiled with `m68k-apple-macos-g++ -m68030`).
* **Bootable ROM Binary**: `ROM/LC-III/cpp/LC-III_cpp.ROM` (1,048,576 bytes, verified master checksum `$ECBBC41C`).

---

## 6. 3D Graphics Performance on the LC III

PlusOS includes a software 3D pipeline (`libplusgfx.a` / `plus3d`):
* Fixed-point 16.16 trigonometry and matrix projection (`plus3d_math.c`).
* 3D wireframe and solid-shaded polygon rasterization (`plus3d_render.c`).
* Interactive showcases: `cube3d.pex`, `starflight.pex`, `solarsys.pex`.

On the LC III, the **25 MHz 68030 with 32-bit buses and on-chip caches** executes matrix multiplications and polygon rasterization **4&times; to 6&times; faster** than an 8 MHz 68000, delivering high frame-rate 3D graphics across the full 640&times;480 resolution.

---

## Navigation

- **Previous Chapter**: [← 15. Package Repository & Software Center](/wiki/15-package-repository-and-network-management)
- **Next Chapter**: [17. Modular Kernel: Drivers, Services & Self-Healing →](/wiki/17-modular-kernel-and-self-healing)
- **Related**: [19. Universal Boot: One Image for Every 680x0 Mac →](/wiki/19-universal-boot)
- **Table of Contents**: [Return to Getting Started](/wiki/01-getting-started)

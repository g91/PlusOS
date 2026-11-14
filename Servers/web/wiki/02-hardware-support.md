# Hardware Support & ROM Architectures

PlusOS 3.1 supports vintage Motorola 68000 and 68030 hardware across the **Macintosh Plus**, **Macintosh SE**, **Macintosh Classic**, and the **Macintosh LC III** (and LC III+ / Performa 450). The kernel dynamically probes the machine hardware, CPU model, and ROM version at boot, tailoring driver dispatch and display resolution accordingly.

---

## Machine Compatibility Matrix

| Feature | Macintosh Plus | Macintosh SE & Classic | Macintosh LC III / Performa 450 |
|---|---|---|---|
| **CPU** | Motorola 68000 @ 8 MHz (16-bit) | Motorola 68000 @ 8 MHz (16-bit) | Motorola 68030 @ 25 MHz (32-bit Bus) |
| **ROM Size** | 128 KB | 256 KB / 512 KB | 1 MB (32-bit Clean) |
| **ROM Checksum / ID** | `$0075` | `$0276` | `$067C` |
| **Display Resolution** | 512 &times; 342 (64 bytes/row) | 512 &times; 342 (64 bytes/row) | 640 &times; 480 (80 bytes/row, Sonora) |
| **Keyboard / Mouse** | Serial protocol (VIA 6522) | Apple Desktop Bus (ADB) | Apple Desktop Bus (ADB) |
| **SCSI Controller** | NCR 5380 (polled pseudo-DMA) | NCR 5380 (hardware handshake) | NCR 53C96 High-Speed SCSI |
| **RAM Limits** | 1 MB min &ndash; 4 MB max (30-pin) | 1 MB min &ndash; 4 MB max (30-pin) | 4 MB onboard &ndash; up to 36 MB (72-pin) |
| **Toolbox Trap Base** | `$000C00` | `$000E00` | Dynamic 32-bit Vector Table |
| **Compiler Target** | `MACHINE=plus` (`-m68000`) | `MACHINE=se` (`-m68000`) | `MACHINE=lc3` (`-m68030`) |

---

## Architecture: Macintosh LC III (68030)

The **Macintosh LC III** introduces significant architectural upgrades over the compact Macs:
1. **Full 32-bit Data & Address Bus**: Unlike the original LC and LC II (which were bottle-necked by a 16-bit bus), the LC III provides a full 32-bit data path running at 25 MHz with on-chip 256-byte instruction and data caches.
2. **Sonora Built-in Video Controller**: Provides standard 640 &times; 480 resolution (80 bytes per row = 38,400 bytes in 1-bit mode) or 8-bit/16-bit color. PlusOS dynamically detects the screen dimensions via `ScreenRow` (`LM_WORD(0x82A)`) and automatically adapts console text geometry (80 columns &times; 60 rows).
3. **High-Speed SCSI (NCR 53C96)**: Replaces the older NCR 5380 with a buffered 53C96 core supporting fast synchronous and asynchronous SCSI-1/2 transfers.
4. **Expanded Memory**: Supports up to 36 MB of 72-pin SIMM RAM with complete 32-bit clean physical addressing.

---

## Memory Map

On 68000 systems (Plus and SE), the address space is 24-bit (up to 16 MB max), while the LC III supports 32-bit addressing:

```text
$000000 - $0003FF   68000 / 68030 Vector Table (Reset, Traps, A-Line, Exceptions)
$000400 - $000EFF   Macintosh Low Memory Globals & Trap Tables
$000F00 - $000FFF   PlusOS API Service Table Pointer & Trap 1 vectors
$001000 - $02FFFF   ROM System Heap & QuickDraw Port structures
$030000 - $05FFFF   PlusOS Userland Applications / Ring-2 Tasks
$060000 - $07FFFF   PlusOS Ring-0 Kernel Code, BSS, and Slab Caches
$080000 - BufPtr    RAM Disk & Dynamic Kernel Free Heap
BufPtr  - MemTop    Video Framebuffer (512x342 or 640x480) & Sound Buffers
```

---

## Compiler Definitions & Targeting

To compile PlusOS for a specific hardware architecture, pass the `MACHINE` variable to `make` or select it directly from the **PlusOS Disk Studio** UI:

- **Macintosh SE (Default)**:
  ```bash
  make install MACHINE=se
  ```
  Passes `-DTARGET_MACSE -m68000` (512&times;342 display, ADB).

- **Macintosh LC III**:
  ```bash
  make install MACHINE=lc3
  ```
  Passes `-DTARGET_MACLC3 -DPLUSOS_MACHINE_LC3 -m68030 -DFB_DEFAULT_WIDTH=640 -DFB_DEFAULT_HEIGHT=480 -DFB_DEFAULT_ROWBYTES=80`.

- **Macintosh Plus**:
  ```bash
  make install MACHINE=plus
  ```
  Passes `-DTARGET_MACPLUS -m68000` (serial keyboard translation).

- **Macintosh Classic**:
  ```bash
  make install MACHINE=classic
  ```
  Passes `-DTARGET_MACCLASSIC -m68000` (ADB, 512&times;342).

- **Universal (one image for every Mac)**:
  ```bash
  make install MACHINE=universal
  ```
  Passes `-DTARGET_UNIVERSAL -DPLUSOS_MACHINE_UNIVERSAL -m68000` with compact-Mac-safe defaults (512&times;342, 64 bytes/row). Builds a **single** `PlusOS.hda` that boots a Plus, SE, Classic **or** LC III, detecting the CPU and hardware at run time via `CPUFlag` (`$012F`). See **[19. Universal Boot](/wiki/19-universal-boot)**.

---

## Navigation

- **Previous Chapter**: [← 01. Getting Started with PlusOS](/wiki/01-getting-started)
- **Next Chapter**: [03. Bootloader & Kernel Relocation →](/wiki/03-bootloader-and-startup)
- **Table of Contents**: [Return to Getting Started](/wiki/01-getting-started)

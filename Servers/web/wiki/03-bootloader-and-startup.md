# Bootloader & Kernel Relocation

PlusOS boots directly from standard Macintosh HFS disk blocks without requiring an existing System Folder, Finder, or Mac OS installation.

---

## The Boot Sequence

The startup process follows an elegant 5-stage lifecycle:

```text
+-----------------------+
|  Macintosh ROM POST   |  Checks RAM, initializes hardware registers
+-----------+-----------+
            |
            v
+-----------------------+
| HFS Boot Blocks (1KB) |  Sector 0 & 1 loaded into memory at $005000;
|     `boot/loader.S`   |  Executes raw assembly loader
+-----------+-----------+
            |
            v
+-----------------------+
|  Kernel Block Loader  |  Reads 512 raw sectors (256 KB) from disk
|                       |  Places raw kernel below `BufPtr`
+-----------+-----------+
            |
            v
+-----------------------+
| Relocation & Startup  |  `crt0.S` computes delta D0 = PC - 2;
|                       |  Applies reloc table, clears BSS, invokes main()
+-----------+-----------+
            |
            v
+-----------------------+
| ROM & Kernel Subsys   |  Initializes heaps, event queues, drivers,
|                       |  starts preemptive scheduler and init task
+-----------------------+
```

---

## Stage 1: The Universal Boot Block (`boot/loader.S`)

Standard Macintosh volumes reserve the first two 512-byte logical sectors (sectors 0 and 1) as the **Boot Blocks**. When the ROM finds a bootable disk:
1. It validates the `$4C4B` ("LK") signature at byte 0.
2. It verifies the entry point offset at byte 2.
3. It jumps to the code inside the boot blocks in supervisor mode.

In PlusOS, `boot/loader.S` is hand-optimized assembly limited to 1,024 bytes (currently compiling to 572 bytes). It is engineered to boot across both classic 68000 Macs (Plus, SE, Classic) and 32-bit clean 68030 Universal ROM Macs (LC III / Performa 450):
- **Universal Register ABI & Drive Resolution**: Reads boot parameters directly from ROM low-memory globals `BootDrive` (`$0210`, word) and `BtDskRfn` (`$0B34`, word), falling back to traversing the drive queue at `DrvQHdr` (`$030A`). This ensures rock-solid drive identification across all Macintosh ROM versions without uninitialized register dereferences.
- **Chunked SCSI Transfer Loop**: Reads the 256 KB kernel in 8 contiguous chunks of 32 KB (64 disk sectors each). This prevents 16-bit DMA / transfer counter overflow on the **NCR 53C96** Async SCSI controller present on the Macintosh LC III, ensuring 100% compatibility across all SCSI and floppy controllers.
- **32-Bit Clean Alignment**: Aligns the load destination buffer below `BufPtr` using 32-bit masking (`andi.l #0xFFFFFFF0, %d0`).
- **68030 Cache Coherency**: On 68020/68030 processors, after reading the kernel into RAM via the SCSI driver, the bootloader flushes the CPU's on-chip instruction and data caches via `movec d0, cacr` before jumping to `_start`. This prevents the 68030 from executing stale cache lines.
- **Real-Time Visual Telemetry**: Renders 12 high-contrast solid black progress stripes (Milestone 1, Milestone 2, Chunks 0..7, Signature `'PLOS'`, and Kernel Jump) across the top rows of the screen as each step succeeds. If a hardware failure occurs, a wide diagnostic pattern (solid white for missing drive, alternating stripes for read error, checkered for magic mismatch) fills rows 4–120.

---

## Persistent Hard Drive Logging (`:boot.log`)

To simplify debugging on physical vintage hardware, PlusOS automatically streams and records the entire boot lifecycle to a permanent log file on the boot volume:
- **Streaming Kernel Buffer**: All early console `printf` and `con_write` output is piped into an 8 KB circular buffer (`klog.c`).
- **Automatic Disk Flushing**: As soon as the File Manager mounts the boot volume (`MountVol`), the kernel flushes the log buffer to `:boot.log` at the root of the drive.
- **Crash & Panic Preservation**: If an unexpected exception, bus error, or kernel panic occurs, the CPU register state, program counter, and stack trace are appended and committed to `:boot.log` prior to dropping into the recovery shell.
- **BlueSCSI Inspection**: When running on BlueSCSI or real SCSI drives, users can pop the SD card into a modern PC or mount the volume in PlusOS Disk Studio to view `:boot.log` directly.

---

## BlueSCSI Configuration Guidelines

When booting a **Macintosh LC III** or any Mac with an **NCR 53C96** high-speed SCSI controller:
- Inspect `bluescsi.ini` on your SD card.
- **Do not enable `System = "MacPlus"`**: The `MacPlus` preset instructs BlueSCSI to simulate the slow, bit-banged handshake delays needed by the 8 MHz Mac Plus NCR 5380. On an LC III, this causes the 53C96 SCSI-2 core to stall during block transfers.
- Leave `System` commented out or set to `Auto` to enable standard SCSI-2 handshakes and parity.

---

## Stage 2: Kernel Self-Relocation (`crt0.S`)

The PlusOS kernel is compiled and linked with position-independent relocation tables emitted by `tools/mkreloc.py`:
1. `_start` calculates the load offset:
   $$\Delta = \text{Actual PC} - \text{Linked PC}$$
2. It iterates through the packed 32-bit relocation fixup table, adding $\Delta$ to every pointer reference in the code and data segments.
3. It zeroes the `.bss` segment.
4. It initializes the C constructor runtime (`__plusos_run_ctors()`) and jumps to `__plusos_kernel_start()`.

---

## Stage 3: Initializing the Macintosh ROM

Because PlusOS runs bare-metal, it must initialize ROM subsystems the exact way the Macintosh System file would:
- **Heaps**: Calls `SetApplBase` to define the application heap boundaries.
- **Event Queue**: Calls `InitEvents` to prepare mouse and keyboard queues.
- **File Manager**: Invokes `InitFS` and `MountVol` to mount the boot HFS volume.
- **QuickDraw**: Initializes the QuickDraw globals below A5 (`InitGraf`).
- **Keyboard**:
  - On the **Mac Plus**: Installs custom key translation procedure in software because the Plus ROM relied on the System file for key translation.
  - On the **Mac SE**: Attaches to the ROM ADB driver's `KeyMap`.

---

## Stage 4: Multi-Boot Selection & Dual-Boot System (`multiboot.cfg`)

Before launching the user environment, the kernel inspects the root volume for `:multiboot.cfg`:
- If configured for **Multi-Boot**, an interactive graphical console menu presents available targets:
  - `PlusOS Desktop (Finder UI)`
  - `PlusOS Command Shell`
  - `Mac OS System 7.5.5 / 7.1 / 6.0.8 (SCSI IDs 1–3)`
- A configurable countdown timer (default 5 seconds) allows hands-free booting into the default choice. Pressing any arrow key halts the timer.
- If a Mac OS target is selected, PlusOS switches the SCSI boot initiator target and hands control back to ROM boot vectors.
- In System 7, users can return to PlusOS at any time by double-clicking the **Return to PlusOS** DA/Applet injected directly by PlusOS Disk Studio.

---

## Floppy Disk Startup (`PlusOS_Floppy.dsk`)

When booted from an 800 KB (DSDD) or 1.44 MB (HD) floppy diskette:
1. The boot blocks load a stripped-down, high-efficiency kernel into RAM.
2. The system bypasses games and large demo binaries to fit within strict floppy capacity limits (<800 KB for standard Mac Plus / SE Sony drives, or up to 1.44 MB for SE FDHD SuperDrives).
3. The system automatically launches the guided hard drive partitioner and setup utility: `installer.pex`.
4. The installer scans SCSI targets, formats the destination drive with a clean HFS filesystem, sets up boot blocks, and installs the base operating system.

For deep-dive setup details, see:
- **[PlusOS Disk Studio & Multi-Threaded Compiler Guide](/wiki/13-plusos-disk-studio-and-compiler)**
- **[Bootable Floppy Disk & Installer Guide](/wiki/14-bootable-floppy-and-installer)**

---

## Navigation

- **Previous Chapter**: [← 02. Hardware Support & ROM Architectures](/wiki/02-hardware-support)
- **Next Chapter**: [04. Kernel Core & Dual-Ring Execution →](/wiki/04-kernel-architecture)
- **Table of Contents**: [Return to Getting Started](/wiki/01-getting-started)


# PlusOS: How to Make a Bootable System & Complete Boot Pipeline

> **Operating System:** PlusOS  
> **Source Modules:** `boot/loader.S`, `kernel/main.c`, `kernel/rominit.c`, `tools/install.py`, `build.bat`  
> **Target Devices:** BlueSCSI v2 SD Card, SCSI2SD, Floppy Emu, Mini vMac emulator  

---

## 1. End-to-End Boot Flow Architecture

Booting PlusOS is a multi-phase transition from hardware reset to raw assembly boot block execution, kernel relocation, ROM subsystem initialization, and finally Ring 2 application execution:

```
+-------------------------------------------------------------------------+
| Phase 1: Hardware Power-On & ROM Self-Test                              |
|   CPU fetches ResetSP ($0000) and ResetPC ($0004) from ROM ($400000).   |
|   ROM runs UST diagnostics, sizes RAM (MemTop), sets up low-memory.     |
+-------------------------------------------------------------------------+
                                    |
                                    v
+-------------------------------------------------------------------------+
| Phase 2: Boot Device Search & Boot Block Loading                        |
|   ROM interrogates Floppy drives, then SCSI IDs 6 down to 0.             |
|   Reads Sectors 0 & 1 of the first Apple_HFS partition into RAM.        |
|   Verifies bbID == $4C4B ('LK') and bbVersion high byte == $44.         |
|   Executes JSR to bbEntry (start of boot/loader.S).                     |
+-------------------------------------------------------------------------+
                                    |
                                    v
+-------------------------------------------------------------------------+
| Phase 3: PlusOS Primary Boot Loader (boot/loader.S)                      |
|   Locates boot drive in ROM Drive Queue (DrvQHdr at $030A).             |
|   Finds driver refNum from queue entry.                                 |
|   Reads KERNEL_BYTES (192 KB) from KERNEL_OFFSET (Block 60000) via raw  |
|   ROM driver _Read calls directly into RAM just below BufPtr.          |
|   Jumps directly to kernel entry point (__plusos_kernel_start).         |
+-------------------------------------------------------------------------+
                                    |
                                    v
+-------------------------------------------------------------------------+
| Phase 4: PlusOS Kernel Entry & Subsystem Bringup (kernel/main.c)        |
|   crt0 relocates kernel code, zeroes BSS, initializes A5 world.         |
|   Installs 68000 exception handlers (exc.S, aline.S, irq.S).            |
|   Initializes ROM subsystems in sequence:                               |
|     1. Heaps: SetApplBase (SysZone 96KB, ApplZone up to stack)          |
|     2. Event Queue: InitEvents(20)                                      |
|     3. File Manager: InitFS(10) + MountVol(g_boot_drive)                |
|     4. QuickDraw: InitGraf + OpenPort on live screen ($0824)            |
|   Initializes Keyboard (Plus translation proc vs SE ADB keymap).        |
|   Initializes Audio hardware (sound_init) and Video driver (video_init).|
|   Wires PlusOSAPI table at $00000F00.                                   |
+-------------------------------------------------------------------------+
                                    |
                                    v
+-------------------------------------------------------------------------+
| Phase 5: Multitasking Scheduler & Userland Launch (init_task)           |
|   Spawns kernel tasks: init_task and heartbeat_task.                   |
|   Checks keyboard Option key ($3A) in KeyMap:                           |
|     If Option held or config set to Shell -> spawns :bin:shell          |
|     Otherwise -> spawns :bin:desktop                                    |
|   Switches CPU to 68000 User Mode (SR = $0000) via RTE.                |
|   PlusOS is fully operational!                                          |
+-------------------------------------------------------------------------+
```

---

## 2. Anatomy of the Boot Block (`boot/loader.S`)

The Macintosh ROM expects the first two 512-byte sectors (1024 bytes total) of a bootable volume to contain a standardized Macintosh Boot Block:

```c
struct BootBlockHeader {
    uint16_t bbID;          /* +$00: Magic number $4C4B ('LK') */
    uint16_t bbEntry_bra;   /* +$02: BRA.W instruction to bootloader code */
    uint16_t bbVersion;     /* +$06: Boot block version. High byte must be $44 */
    uint16_t bbPageFlags;   /* +$08: Page flags */
    uint8_t  bbSysName[16]; /* +$0A: Pascal string "System" (unused by PlusOS) */
    uint8_t  bbShellName[16];/*+$1A: Pascal string "Finder" (unused by PlusOS) */
    uint8_t  bbDbg1Name[16];/* +$2A: "MacsBug" */
    uint8_t  bbDbg2Name[16];/* +$3A: "Disassembler" */
    uint8_t  bbScreenName[16];/*+$4A: "StartUpScreen" */
    uint8_t  bbHelloName[16];/*+$5A: "Finder" */
    uint8_t  bbScrapName[16];/*+$6A: "Clipboard" */
    int16_t  bbCntFCBs;     /* +$7A: Max open files (10) */
    int16_t  bbCntEvts;     /* +$7C: Event queue capacity (20) */
    uint32_t bb128KSHeap;   /* +$7E: System heap size on 128K Mac */
    uint32_t bb256KSHeap;   /* +$82: System heap size on 256K Mac */
    uint32_t bbSysHeapSize; /* +$86: System heap size on 512K+ Mac ($20000 = 128KB) */
};
```

### The `$44xx` Version Trick
When the Mac ROM examines the boot blocks:
- If `bbVersion` is `$44xx` (such as `$4418`), the ROM **does not** look for a classic `System` or `Finder` file.
- Instead, the ROM immediately executes a subroutine jump (`JSR`) directly to `bbEntry` (`start` in `loader.S`) in Supervisor Mode!
- Because PlusOS never returns from `start`, it bypasses the entire classic Mac OS boot path.

---

## 3. How `loader.S` Reads the Kernel from Disk

At the time `loader.S` runs, the Mac File Manager **does not exist yet**. Therefore, `loader.S` must communicate with the disk controller using the boot drive's raw driver:

```assembly
start:
    movem.l %d2-%d7/%a2-%a6, -(%sp)

    /* 1. Retrieve the boot drive number from low-memory global BootDrive ($0210) */
    move.w  0x0210, %d7                 /* BootDrive */

    /* 2. Walk the ROM Drive Queue (DrvQHdr at $030A) to find the driver refNum */
    movea.l 0x030A, %a0                 /* DrvQHdr.qHead */
find_drive:
    cmp.w   6(%a0), %d7                 /* Compare qElement->drvNum with BootDrive */
    beq.s   found_drive
    movea.l (%a0), %a0                  /* qElement->qLink (next entry) */
    bra.s   find_drive

found_drive:
    move.w  8(%a0), %d6                 /* d6 = driver refNum (e.g. -32 for SCSI) */

    /* 3. Determine load destination: place kernel just below BufPtr ($012C) */
    movea.l 0x012C, %a4                 /* BufPtr (top of usable RAM) */
    suba.l  #KERNEL_BYTES, %a4          /* a4 = destination buffer in RAM */

    /* 4. Issue raw ROM driver _Read calls (Trap $A002) */
    lea     io_param_block, %a0
    move.w  %d6, 24(%a0)                /* ioRefNum */
    move.l  %a4, 32(%a0)                /* ioBuffer = target in RAM */
    move.l  #KERNEL_BYTES, 36(%a0)      /* ioReqCount (192 KB) */
    move.l  #KERNEL_OFFSET, 46(%a0)     /* ioPosOffset (Sector 60000 * 512) */
    move.w  #1, 44(%a0)                 /* ioPosMode = fsFromStart */
    .word   0xA002                      /* _Read trap */

    /* 5. Jump directly into the kernel entry point */
    jmp     (%a4)                       /* Never returns! */
```

---

## 4. Kernel Subsystem Bringup Sequence (`kernel/rominit.c`)

When the kernel takes control at `__plusos_kernel_start`, it brings up the ROM subsystems in the exact order required by the Mac hardware:

```c
void rom_init(void)
{
    /* 1. Detect Boot Drive and SCSI Driver refNum */
    g_boot_drive = (int16_t)LM_WORD(0x210);
    for (uint8_t *q = (uint8_t *)LM_LONG(0x30A); q; q = *(uint8_t **)q) {
        if (*(int16_t *)(q + 6) == g_boot_drive) {
            g_boot_refnum = *(int16_t *)(q + 8);
            break;
        }
    }

    /* 2. Configure System Heap and Application Zone */
    init_heap();

    /* 3. Initialize OS Event Queue (20 entries) */
    init_events();

    /* 4. Initialize File Manager and mount the boot HFS volume */
    init_files();

    /* 5. Set up QuickDraw globals and GrafPort over live screen ($0824) */
    init_quickdraw();
}
```

### Memory Map at Kernel Start
```
Physical RAM End (e.g., $400000 on 4MB Mac)
+-------------------------------------------------------+ $400000
| Sound Buffer (370 bytes) & Scratchpad                 | $3FFD00
+-------------------------------------------------------+ $3FA700
| Live Screen Frame Buffer (21,888 bytes, 512x342)      | $3FA700 (ScrnBase)
+-------------------------------------------------------+ $3FA700
| BufPtr (Allocated below screen buffer)                |
+-------------------------------------------------------+
| PlusOS Kernel Code & Data Image (~192 KB)             |
+-------------------------------------------------------+
| Kernel Supervisor Stack (KERNEL_STACK = 8 KB)         |
+-------------------------------------------------------+
| Application Heap (ApplZone)                           | Dynamic expansion
|   Allocated via plusrom_set_appl_base()               | via MaxApplZone()
+-------------------------------------------------------+
| System Heap (SysZone, 96 KB)                          | $001000 - $019000
+-------------------------------------------------------+
| Low-Memory Globals & CPU Vectors ($0000 - $0FFF)      | $000000 - $000FFF
+-------------------------------------------------------+ $000000
```

---

## 5. Step-by-Step Guide: Creating a Bootable Disk Image

### Prerequisites
- Windows Command Prompt or PowerShell
- WSL (Ubuntu) with `m68k-elf-gcc` or `m68k-linux-gnu-gcc`
- Python 3 with `hfsutils` installed (`sudo apt install hfsutils`)

### Step 1: Compile the SDKs, Loader, Kernel and Apps
From `C:\Users\Evil\PlusOS`:
```cmd
build.bat
```
This runs the Makefile inside WSL:
1. Builds `sdks/macplus` (`libplusrom.a`, `libplusos.a`, `libplusnet.a`, `libplusgfx.a`, `libc.a`).
2. Builds `sdks/macse` (`libserom.a`).
3. Compiles `boot/loader.S` -> `build/loader.bin` (raw 1024-byte binary).
4. Compiles `kernel/` -> `build/kernel.pex` (relocatable 68000 binary).
5. Compiles all applications in `apps/` (`desktop.pex`, `shell.pex`, `cube3d.pex`, etc.).

### Step 2: Install onto a Disk Image (`tools/install.py`)
Run:
```cmd
build.bat install
```
Or execute `tools/install.py` directly:
```cmd
python tools/install.py --image F:\HD60_512 PlusOS.hda ^
                        --loader build/loader.bin ^
                        --kernel build/kernel.pex ^
                        --hfs
```

### What `install.py` Does Under the Hood:
1. **Locates Apple_HFS Partition**:
   - Reads Sector 0: Verifies Apple Driver Descriptor (`b"ER"`).
   - Reads Partition Map (Sector 1..N): Finds the partition named `Apple_HFS` with magic `b"PM"`.
2. **Writes Boot Blocks (Blocks 0 & 1)**:
   - Injects `loader.bin` at offset 0 of the HFS partition.
3. **Writes Raw Kernel Image (Block 60000)**:
   - Writes `kernel.pex` starting at partition block 60,000 (30 MB into partition).
4. **Reserves Allocation Blocks in HFS Volume Bitmap**:
   - Marks blocks 60,000 through 64,000 as **allocated** in the HFS Volume Allocation Map (`vbm_st`). This guarantees that classic Mac OS tools and standard file operations will **never overwrite the kernel**.
5. **Copies Applications via HFS**:
   - Uses `hcopy` from `hfsutils` to copy `shell.pex`, `desktop.pex`, `cube3d.pex`, and utilities into `:bin:` and volume root so the PlusOS shell and desktop can locate them as standard files.

### Step 3: Test Boot in Mini vMac
```cmd
build.bat test
```
This launches `tools/vmac.ps1`, loading `Mini vMac.exe` with `vMac.ROM` and mounting `HD60_512 PlusOS.hda`. The system boots directly to the PlusOS Desktop or Shell in under 2 seconds!

### Step 4: Boot on Real Hardware (BlueSCSI v2)
1. Copy `HD60_512 PlusOS.hda` onto the root of a micro-SD card formatted with FAT32 or exFAT.
2. (Optional) Configure `bluescsi.ini` with WiFi credentials if using DaynaPORT networking.
3. Insert the micro-SD card into your BlueSCSI v2 attached to the SCSI bus of a Macintosh Plus or Macintosh SE.
4. Power on the Macintosh. The Mac chime sounds, the SCSI bus is scanned, BlueSCSI responds at ID 6, the ROM reads blocks 0 & 1, executes `loader.S`, loads the kernel into RAM, and boots PlusOS!

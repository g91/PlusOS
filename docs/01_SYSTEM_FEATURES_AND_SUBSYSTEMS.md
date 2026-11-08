# PlusOS: System Features, Subsystems & Architecture

> **Operating System:** PlusOS (Bare-Metal 68000 OS for Macintosh Plus & Macintosh SE)  
> **Source Location:** `C:\Users\Evil\PlusOS`  
> **Target Hardware:** Motorola 68000 CPU, 1–4 MB RAM, 512×342 1-bit Monochrome Display, NCR 5380 SCSI, BlueSCSI v2  

---

## 1. System Overview & Philosophy

**PlusOS** is an independent, bare-metal operating system engineered specifically for the classic Motorola 68000-based Macintosh Plus (128 KB ROM) and Macintosh SE (256 KB ROM). Rather than booting Apple's System 6 or System 7 Finder environment, PlusOS takes control of the machine immediately from the disk boot blocks, establishes its own kernel, multitasking scheduler, virtual terminal console, graphical desktop window server, network stack, and executable loader, while leveraging the hardware-tuned routines embedded in the Macintosh ROM.

```
+-------------------------------------------------------------------------+
|                  Ring 2: Userland Applications & Shell                  |
|    Desktop GUI | Terminal Shell | Task Manager | Files | 3D Engine      |
|    Classic Mac 'APPL' Binaries  | Native PlusOS Executables (.pex)      |
+-------------------------------------------------------------------------+
|         Syscall Gate (TRAP #1)      |     A-Line ROM Trap Gate (EMT 1010)|
|         syscall.h / sys_dispatch    |     aline.S (User -> Super Stack) |
+-------------------------------------+-----------------------------------+
|                  Ring 1: System Services (Supervisor)                   |
|    Window Server (winsrv.c)     | PlusNET TCP/IP Stack (plusnet/)       |
|    HFS File System Service      | Device Driver Registry                |
+-------------------------------------------------------------------------+
|                    Ring 0: PlusOS Kernel (Supervisor)                   |
|  +--------------------------------+  +--------------------------------+ |
|  | Preemptive Task Scheduler      |  | Physical Memory & Stack Alloc  | |
|  | (struct proc, tick.S, switch.S)|  | (Kernel stacks, User stacks)   | |
|  +--------------------------------+  +--------------------------------+ |
|  | Double-Buffered Video Engine   |  | Hardware Sound Synthesizer     | |
|  | (vblit.S 64-byte unrolled scan)|  | (sound.c 4-voice PWM hardware) | |
|  +--------------------------------+  +--------------------------------+ |
|  | Exception & Crash Subsystem    |  | Configuration & Registry DB    | |
|  | (exc.S, crash.c register dump) |  | (config.c, registry.c)         | |
|  +--------------------------------+  +--------------------------------+ |
+-------------------------------------------------------------------------+
|                      Hardware Layer & Macintosh ROM                     |
|    Mac Plus ROM ($0075, 128KB)   |   Mac SE ROM ($0276, 256KB)          |
|    QuickDraw | MemoryMgr | FileMgr | EventMgr | VIA 6522 | SCC 8530    |
+-------------------------------------------------------------------------+
```

---

## 2. The Three Software Privilege Rings

Although the Motorola 68000 CPU provides only two hardware privilege states (Supervisor Mode and User Mode) without an MMU (Memory Management Unit) for page-level memory protection, PlusOS enforces **three software rings**:

| Ring | CPU State | Components | Responsibilities & Privilege Boundaries |
|:---:|:---:|:---|:---|
| **Ring 0** | **Supervisor** (`SR` bit 13 = 1) | Kernel Core (`kernel/`) | Scheduler, context switching, interrupt handlers, exception gates, physical RAM allocation, low-level drivers. |
| **Ring 1** | **Supervisor** (`SR` bit 13 = 1) | System Services | Window server (`winsrv.c`), TCP/IP network stack (`plusnet/`), HFS file services, disk block drivers. |
| **Ring 2** | **User Mode** (`SR` bit 13 = 0) | User Applications (`apps/`) | Desktop shell, games, utilities, classic Mac apps. Cannot execute privileged instructions (`STOP`, `RESET`, `move to SR`). System requests must enter via `TRAP #1` or ROM A-line traps. |

### Ring Boundary Enforcement
Because the 68000 lacks an MMU, Ring 2 processes share the physical address space. However, privilege isolation is enforced via:
1. **Instruction Gating**: Any attempt by a Ring 2 program to execute privileged instructions triggers a Privilege Violation Exception (`vector 8`), which is intercepted by `kernel/exc.S` and cleanly terminates the faulted process back to the shell.
2. **Dedicated User Stack**: Ring 2 applications execute exclusively using the User Stack Pointer (`USP`). The kernel maintains an independent Supervisor Stack Pointer (`SSP`) for every process.
3. **Controlled Gateways**: Communication between userland and the kernel occurs strictly across `TRAP #1` (the syscall gate) and `vector 10` (`$0028`, the A-line trap gate).

---

## 3. Syscall Gate & IPC Mechanism (`syscall.h`, `syscall.c`)

Userland applications invoke kernel services via the 68000 `TRAP #1` instruction (Vector 33 at address `$0084`):

```
Userland (Ring 2)                         Kernel (Ring 0 / 1)
-----------------                         -------------------
D0 = Syscall Number (SYS_*)
A0 = Argument Block Pointer
TRAP #1  -------------------------------> trap1_entry (kernel/trap.S)
                                            Switches to Supervisor Mode
                                            Validates syscall number
                                            Invokes sys_dispatch(num, args)
                                            Places result in D0
RTE      <------------------------------- Restores User Mode & CCR
```

### Core PlusOS Syscalls (`kernel/syscall.h`)

| Syscall | Constant | Parameters | Description |
|:---|:---:|:---|:---|
| `SYS_YIELD` | `0` | None | Voluntarily yields the remainder of the current time slice to the scheduler. |
| `SYS_EXIT` | `1` | `int code` | Terminates the current process, frees its resources, and awakes waiting parent. |
| `SYS_SPAWN` | `2` | `const char *path, int argc, char **argv` | Spawns a new executable image as a child process. |
| `SYS_WAIT` | `3` | `int pid, int *status` | Blocks until the designated child process terminates. |
| `SYS_SLEEP` | `4` | `uint32_t ticks` | Suspends execution for the specified duration (60 Hz ticks). |
| `SYS_TICKS` | `5` | None | Returns the 60 Hz tick counter since system boot. |
| `SYS_SECONDS` | `6` | None | Reads the Real-Time Clock (seconds since Jan 1, 1904). |
| `SYS_PRINT` | `7` | `const char *str, size_t len` | Writes a character buffer to the virtual console. |
| `SYS_GETKEY` | `8` | None | Blocks until a key is pressed; returns ASCII or `KEY_*` code. |
| `SYS_POLLKEY` | `9` | None | Non-blocking check for keyboard input (returns 0 if queue empty). |
| `SYS_ALLOC` | `10` | `uint32_t size` | Allocates a contiguous memory block from the system heap. |
| `SYS_FREE` | `11` | `void *ptr` | Deallocates a previously allocated memory block. |
| `SYS_BLK_READ` | `12` | `uint32_t blk, uint32_t cnt, void *buf` | Reads raw 512-byte disk blocks from the boot partition. |
| `SYS_BLK_WRITE`| `13` | `uint32_t blk, uint32_t cnt, void *buf` | Writes raw 512-byte disk blocks to the boot partition. |
| `SYS_PRESENT` | `14` | None | Flips the offscreen back buffer to the physical CRT raster. |
| `SYS_PRESENT_R`| `15` | `int top, int bottom` | Flips a bounded horizontal band of rows to the CRT. |

---

## 4. Preemptive Multitasking & Scheduler (`kernel/proc.c`, `kernel/switch.S`)

PlusOS features a **hybrid preemptive/cooperative multitasking kernel** supporting up to 16 concurrent processes (`MAX_PROCS = 16`):

```c
struct proc {
    int         pid;
    char        name[16];
    int         state;              /* FREE, RUNNING, READY, SLEEPING, ZOMBIE */
    uint32_t    kstack_bottom;      /* Base of 4 KB kernel supervisor stack */
    uint32_t    ksp;                /* Saved supervisor stack pointer (SSP) */
    uint32_t    usp;                /* Saved user stack pointer (USP) */
    uint32_t    sleep_until;        /* Wakeup tick threshold */
    int         exit_code;          /* Exit code for parent wait() */
    int         parent_pid;         /* PID of parent process */
    uint32_t    runtime_ticks;      /* Total CPU time consumed */
};
```

### Preemption Mechanics (`kernel/tick.S`, `kernel/switch.S`)
1. **Vertical Blank Interrupt (VBL)**: The 6522 VIA triggers Level 1 interrupts at ~60.15 Hz. The interrupt handler (`tick.S`) increments the global `Ticks` counter and decrements the active process's quantum.
2. **Context Switching (`proc_switch` in `kernel/switch.S`)**:
   - Saves registers `D2–D7` and `A2–A6` to the current supervisor stack.
   - Stores current `SP` into `curr_proc->ksp`.
   - Loads `next_proc->ksp` into `SP`.
   - Restores registers `D2–D7` and `A2–A6` from the new process's stack.
   - Executes `RTS` to resume execution in the newly scheduled process context.

---

## 5. Console & Shell Subsystem (`kernel/console.c`, `kernel/shell.c`)

The PlusOS console operates directly on the 512×342 1-bit frame buffer, rendering a **64-column by 42-row** text terminal with custom 8×8 bitmapped fonts.

### Persistent Command History
- Stores up to **20 command lines** in an active ring buffer.
- On reboot, the history is automatically saved to and loaded from `:PlusOS History` on the boot volume.
- Users navigate previous commands using the **Up Arrow** (`KEY_UP = $1E`) and **Down Arrow** (`KEY_DOWN = $1F`) keys.

### Tab Auto-Completion Engine
Pressing **Tab** (`KEY_TAB = $09`) invokes the path and command completion scanner:
1. Scans built-in shell commands (`help`, `ls`, `ps`, `taskman`, `cat`, `calc`, `cube3d`, etc.).
2. Searches executable binaries in `:bin:` and volume root (`:`).
3. Searches directories and files across HFS paths (`:Desktop:`, `:Games:`).
4. Successive Tab presses cycle through all candidate matches.

---

## 6. Graphical Window Server & Desktop GUI (`kernel/winsrv.c`, `apps/desktop/`)

PlusOS contains a lightweight, high-performance window server providing modern desktop capabilities on 1980s hardware:
- **Matrix Wallpaper**: Animated or static digital rain background rasterized on the desktop surface.
- **Movable Windows**: Non-overlapping and overlapping windows with drag title bars, close boxes, and active borders.
- **Desktop Taskbar & Menu Bar**: System menu, running application pills, and a real-time clock.
- **GUI Task Manager (`taskman`)**: Live graphical CPU load meters per task, memory usage breakdown, and process kill controls.
- **Theme Switcher (`desktop_theme.c`)**: Dynamic switching between Classic Mac, Dark Mode, Matrix Green, and Synthwave themes.

---

## 7. Dual Executable Engine (`kernel/exec.c`, `kernel/macloader.c`)

PlusOS provides native support for two distinct executable formats:

### 1. Native PlusOS Executables (`.pex`)
- **Header**: 8-byte header starting with branch opcode `BRA.W _start` (`$6000 xxxx`) followed by magic ASCII `"PLOS"` (`$504C4F53`).
- **Relocation**: Position-independent binaries emitted by `m68k-elf-gcc` linked at address 0. At load time, `crt0.S` reads a compressed relocation table, adds the physical load address delta to all pointers, clears BSS, and executes `main()`.

### 2. Classic Macintosh Applications (`'APPL'` / `'CODE'`)
- Handled by `kernel/macloader.c`, allowing genuine 68000 System 6 and System 7 applications to run directly under PlusOS without booting classic System software.
- Reads `'CODE'` resource 0, parses jump table offsets, builds the **Macintosh A5 World**, and traps `_LoadSeg` (`$A9F0`) to dynamically load additional application code segments on demand.

---

## 8. Network Subsystem: PlusNET (`sdks/macplus/include/plusnet/`)

PlusOS connects to modern local networks and the Internet via a **BlueSCSI v2** hardware device running in WiFi DaynaPORT emulation mode:
- **Hardware Interface**: Emulates a DaynaPORT SCSI/Link network interface on the SCSI bus (typically SCSI ID 4, enabled via an empty `NE4.hda` file).
- **Driver**: Low-level driver (`dayna.c`) executes SCSI Command Descriptor Blocks (`SCSICmd`) to send and receive raw Ethernet frames.
- **Protocols Implemented**:
  - **ARP**: Address Resolution Protocol (RFC 826).
  - **IPv4**: Internet Protocol Version 4 with checksum verification.
  - **ICMP**: Ping Echo Request and Reply (`ping`).
  - **UDP**: User Datagram Protocol.
  - **DHCP Client**: Automatic IP address, subnet mask, gateway, and DNS configuration.
  - **DNS Resolver**: Domain name resolution over UDP port 53.
  - **TCP**: Full state machine (SYN, ACK, FIN, RST, retransmission, sliding window) powering built-in `telnet` and `irc` clients.

---

## 9. Hardware Sound Engine (`kernel/drivers/sound/sound.c`)

Classic compact Macs generate sound through the 6522 VIA and custom analog circuitry that clocks out a byte of audio sample data during each horizontal blanking interval (~22,255 Hz):
- PlusOS provides a **4-voice PWM tone and wave synthesizer**.
- Supports square waves, triangle waves, noise generation, and frequency ramps for games and alert sounds.
- Syscall `plusos->sound_play(freq, duration_ticks)` allows user applications to produce asynchronous non-blocking audio alerts.

---

## 10. Modular Repository & Subsystem Directory Topology

PlusOS organizes its kernel and application ecosystem into clean, modular subdirectories:

### Kernel Architecture (`kernel/`)
* **`kernel/core/`**: Central kernel entry point (`main.c`), ROM bringup sequence (`rominit.c`), Classic Mac OS chainloading (`chainboot.c`), system shutdown/reboot (`sys.c`), and master API dispatch table (`api.c`, `kernel.h`).
* **`kernel/cpu/`**: Low-level 68000 CPU gates: Line 1010 A-line trap gate (`aline.S`), `TRAP #1` syscall dispatcher (`trap.S`, `syscall.c`, `syscall.h`), CPU exception vectors (`exc.S`), hardware interrupts (`irq.S`), and crash register dump (`crash.c`).
* **`kernel/proc/`**: Preemptive multitasking process scheduler (`proc.c`, `proc.h`), assembly context switcher (`switch.S`), and VBL timer tick handler (`tick.c`, `tick.S`).
* **`kernel/drivers/`**: Hardware device drivers:
  * `video/`: Framebuffer presentation (`video.c`), 16-longword unrolled scanline blitter (`vblit.S`), 64×42 virtual text console (`console.c`), and QuickDraw demo primitives (`gfx.c`).
  * `input/`: Mac Plus & SE ADB keyboard handler (`kbd.c`), keymaps (`keymap.c`), and translation routine (`keytrans.S`).
  * `sound/`: 4-voice PWM audio driver (`sound.c`).
* **`kernel/services/`**: High-level system services: binary executor (`exec.c`), Classic Mac `'APPL'` A5 World loader (`macloader.c`), graphical window server (`winsrv.c`), boot recovery menu (`menu.c`), kernel shell (`shell.c`), scrap clipboard (`scrap.c`), configuration (`config.c`), and system registry (`registry.c`).

### Categorized Applications Ecosystem (`apps/`)
All 54 programs are separated into dedicated category folders:
* **`apps/games/`** (13 games): `asteroids`, `breakout`, `dungeon`, `flappy`, `invaders`, `lunar`, `mines`, `pacman`, `puzzle`, `racer`, `snake`, `starflight`, `tetris`.
* **`apps/development/`** (14 programs): `apple2e`, `basic`, `c64`, `c128`, `chip8`, `cube3d`, `emu6502`, `emu8080`, `hello`, `hexedit`, `macrun`, `qdemo`, `solarsys`, `spin`.
* **`apps/tools/`** (21 utilities): `calc`, `cat`, `clear`, `clip`, `date`, `echo`, `files`, `help`, `kill`, `ls`, `mem`, `notes`, `ps`, `reg`, `ret`, `sleep`, `sound`, `sysinfo`, `taskman`, `theme`, `ver`.
* **`apps/network/`** (4 clients): `irc`, `net`, `ping`, `telnet`.
* **`apps/system/`** (6 core apps): `boot`, `desktop`, `off`, `reboot`, `shell`, `startup`.

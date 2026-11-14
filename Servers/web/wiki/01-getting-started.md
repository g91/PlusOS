# Getting Started with PlusOS

**PlusOS 3.1** is a clean-sheet, bare-metal operating system for the Motorola 680x0-based Apple Macintosh — the Macintosh Plus, SE, and Classic (68000) and the LC III (68030). It was designed and created by **the1Domo**.

> **New in 3.1 — the Universal Loader release.** A single `PlusOS.hda` now boots any supported machine, detecting the CPU and hardware at run time (just as System 7 does) instead of needing a per-machine build. Version 3.1 also introduces a **modular, self-healing kernel** — drivers and services run as supervised modules that are isolated and restarted on failure — and an **in-kernel crash debugger** that records a post-mortem dump and recovers instead of resetting the Mac. See **[17. Modular Kernel](/wiki/17-modular-kernel-and-self-healing)**, **[18. Crash Debugger](/wiki/18-crash-debugger-and-post-mortem-dumps)**, and **[19. Universal Boot](/wiki/19-universal-boot)**. The previous release is archived at **[plusos.d0a.net/2.0](https://plusos.d0a.net/2.0)**.

Unlike standard vintage Macintosh software, PlusOS does not run inside System 6 or System 7. Instead, it takes over the hardware at boot block execution, interacts directly with the Macintosh Toolbox ROM, and provides a modern multi-process Unix-like runtime with a preemptive kernel, dynamic memory management, TCP/IP networking, 3D graphics, and a flexible Desktop GUI.

---

## Creator & Vision

PlusOS is created and maintained by **the1Domo**. The goal was to prove what the classic Macintosh hardware is truly capable of when freed from single-tasking constraints and legacy OS baggage:
- True **preemptive multitasking** on an un-MMU'd 8 MHz Motorola 68000.
- One **Universal Loader** image that boots a Plus, SE, Classic, or LC III, detecting the CPU at run time like System 7.
- A **modular, self-healing kernel**: drivers and services run as supervised modules that are isolated and restarted on failure instead of panicking.
- Dynamic **RAM management** with slab allocators, heap canaries, and an automatic garbage collector.
- Built-in **TCP/IP networking** driving modern WiFi via BlueSCSI DaynaPORT emulation.
- Modern desktop conveniences: persistent command history, tab completion, multi-OS theme switching, and live window dragging.
- Native backward compatibility: executing Classic Macintosh `'APPL'` binaries directly through Jump Table and A5 World synthesis.

Official distribution site: **[https://plusos.d0a.net](https://plusos.d0a.net)**

---

## Hardware Requirements

PlusOS runs on original vintage hardware and modern emulators:

| Machine | CPU | Clock | Supported ROMs | Min RAM | Recommended |
|---|---|---|---|---|---|
| **Macintosh Plus** | 68000 | 8 MHz | 128 KB ($0075) | 1 MB | 4 MB |
| **Macintosh SE** | 68000 | 8 MHz | 256 KB ($0276) | 1 MB | 4 MB |
| **Macintosh SE FDHD** | 68000 | 8 MHz | 256 KB ($0276) | 1 MB | 4 MB |
| **Macintosh LC III / Performa 450** | 68030 | 25 MHz | 1024 KB ($067C) | 4 MB | 4–36 MB |
| **Mini vMac** | Emulated 68000 | 1x - 32x | Plus / SE ROMs | 4 MB | 4 MB |

### Storage & Peripherals
- **BlueSCSI V2** (Recommended): Enables high-speed SCSI emulation and wireless internet via Raspberry Pi Pico W WiFi.
- **SCSI2SD / RaSCSI / ZuluSCSI**: Fully compatible for disk storage.
- **Original SCSI Hard Drives**: Apple 20SC, 40SC, 80SC, Quantum ProDrive.

---

## Quick Start: Flashing the Disk Image

1. Download the latest `PlusOS.hda` from **[plusos.d0a.net](https://plusos.d0a.net)**.
2. Place the file onto the root of your BlueSCSI MicroSD card:
   - Name it `HD60_512 PlusOS.hda` (sets SCSI ID 6, 512-byte sectors).
3. If using BlueSCSI WiFi networking, configure `bluescsi.ini`:
   ```ini
   [WiFi]
   WiFiSSID=YourNetworkName
   WiFiPassword=YourNetworkPassword
   ```
   and create an empty placeholder file named `NE4.hda` (sets DaynaPORT emulation at SCSI ID 4).
4. Insert the SD card into your BlueSCSI, power on your Macintosh SE or Plus, and watch the system boot!

---

## First Boot Experience

At startup, PlusOS displays its boot banner:
```text
PlusOS 3.1 by the1Domo on the Macintosh SE - 68000, 4096K RAM
Starting the ROM:
Starting the kernel: scheduler, timer, IPC, syscalls
```

Depending on your startup configuration, PlusOS will launch either:
1. **The Desktop GUI**: Featuring multi-window management, the "Programs" launcher, Task Manager, Note Pad, Calculator, and 6 desktop themes.
2. **The Recovery Shell**: A powerful Unix-like CLI with tab completion and persistent history across reboots.

To switch between the Desktop and Shell at any time:
- In the shell: run `desktop`
- On the desktop: select **Special -> Quit Desktop** or launch **Shell** from the Programs menu.
- To set your permanent boot preference: run `startup shell` or `startup desktop`.

---

## Tooling, Floppy Booting & Online Packages

PlusOS 3.1 includes modern companion tools and installation options:

- **[PlusOS Disk Studio & Compiler](/wiki/13-plusos-disk-studio-and-compiler)**: A high-performance native Windows GUI (`PlusOSDiskStudio.exe`) and native Linux CLI/TUI suite (`./PlusOSDiskStudio` / `make studio`) featuring an asynchronous multi-threaded compiler, live build output streaming, HFS disk image explorer, dual-boot menu configurator, and granular software package selection checklist.
- **[Bootable Floppy & System Installer](/wiki/14-bootable-floppy-and-installer)**: For diskette-based startup, the minimal 1.44 MB / 800 KB bootable floppy disk image (`PlusOS_Floppy.dsk`) boots straight into a guided hard drive partitioning and installation wizard (`installer.pex`).
- **[Package Repository & Network Management](/wiki/15-package-repository-and-network-management)**: Download and install software over the air using `pkg install <name>` or the visual `pkgmgr` Desktop package manager, connected to the dedicated dual-mode HTTP (8088) and SSL/TLS HTTPS (8443) package server.
- **[Macintosh LC III ROM Reverse-Engineering & Port](/wiki/16-macintosh-lc3-rom-decompilation-and-port)**: Full architectural breakdown of the 68030 port, decompiled 1024 KB Universal SuperMario ROM (`$067C`), opcode signature correlation, Sonora 640×480 video controller, and NCR 53C96 high-speed SCSI-2.
- **[Modular Kernel & Self-Healing](/wiki/17-modular-kernel-and-self-healing)**: PlusOS as supervised kernel modules — drivers and services that talk over named pipelines, with a supervisor that isolates and restarts anything that fails, so a crash is recovered rather than fatal.
- **[In-Kernel Crash Debugger & Post-Mortem Dumps](/wiki/18-crash-debugger-and-post-mortem-dumps)**: The on-screen debugger panel, full 68000 register capture, the `PlusOS CrashDump` (`PDMP`) format, and the `plusdump` CLI viewer with a built-in 68000 disassembler.
- **[Universal Boot](/wiki/19-universal-boot)**: Build one `PlusOS.hda` with `MACHINE=universal` that boots a Macintosh Plus, SE, Classic **or** LC III, detecting the CPU at run time just as System 7 does.

---

## Navigation

- **Next Chapter**: [02. Hardware Support & ROM Architectures →](/wiki/02-hardware-support)
- **Tooling Guide**: [13. PlusOS Disk Studio & Compiler Pipeline →](/wiki/13-plusos-disk-studio-and-compiler)
- **Installer Guide**: [14. Minimal Bootable Floppy & Hard Drive Installer →](/wiki/14-bootable-floppy-and-installer)


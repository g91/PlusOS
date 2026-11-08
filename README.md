# PlusOS 2.0

A bare-metal operating system for the Macintosh Plus and Macintosh SE,
created by **the1Domo**, written in C on top of the vintage Mac ROM, plus one SDK per machine.

🌐 **Website & Wiki:** [plusos.d0a.net](https://plusos.d0a.net)  
[![Discord](https://img.shields.io/badge/Discord-Join%20Chat-5865F2?style=for-the-badge&logo=discord&logoColor=white)](https://discord.gg/hkpe76G7AU)

<p align="center">
  <img src="docs/dasktop.png" alt="PlusOS running on a real Macintosh SE" width="480"><br>
  <em>PlusOS booted on a real Macintosh SE — the desktop with Matrix wallpaper, the theme switcher, and on-screen icons.</em>
</p>

Source lives on Windows; the m68k cross compiler runs in WSL (Ubuntu) using standard GNU toolchains (`m68k-elf-gcc`, `m68k-linux-gnu-gcc`, or Retro68).
`build.bat` runs everything from a Windows prompt. The build system is fully portable and self-contained within its own directories, not relying on hardcoded external drives or SD cards.

The Macintosh ROM assembly source tree is open-source and available at:
👉 **[https://github.com/jonathanschilling/mac_rom](https://github.com/jonathanschilling/mac_rom)**

```
build              build both SDK libraries, boot loader, kernel, apps
build install      write it all to disk images (local images/ and external SD card if present)
build test         install, then boot it in Mini vMac
build sdks         regenerate both SDK folders from the Plus and SE ROM sources
build sdktest      run both SDKs' self-tests
build clean
```

## PlusOS Disk Studio (GUI OS Builder & Hard Drive Explorer)

Located right in the root of the repository — **`PlusOSDiskStudio.exe`** — this native, high-performance C++ tool lets you customize, compile, dual-boot, and manage Macintosh hard drive and floppy images (`.hda`, `.img`, `.dsk`).

The studio features two distinct, dedicated dialogues:

### 1. OS Builder & Dual-Boot System Configurator
- **Compile & Build Hard Drive Images**: One-click build pipeline running `make install` inside WSL / `build.bat`, writing the boot blocks, kernel, and applications directly to `images/PlusOS.hda` and connected BlueSCSI SD cards (`F:`).
- **Dual-Boot & Multi-Boot System Setup**:
  - Configure the boot behavior: Boot directly into **Mac OS** (System 7.5.5, 7.1, 6.0.8, etc.), **PlusOS Desktop**, **PlusOS Shell**, or the **Interactive Multi-Boot Menu**.
  - Edit boot records (`multiboot.cfg`): Add custom operating system entries, cycle boot target SCSI IDs (`0`–`7`, Shell, Desktop), remove, and reorder entries.
  - Set an automated boot countdown timer (in seconds) or manual choice at startup.
- **Pre-Build OS Customizations & Theming**:
  - **Visual Themes**: Choose default styling from Mac OS 7.1 Platinum, Commodore Amiga Workbench 1.3/2.0, Windows 95, Windows 98, Windows Vista/7 Aero Glass, or Hacker CRT Dark Mode.
  - **Wallpapers**: Select between Classic Mac 50% Gray stipple or live animated Matrix Digital Rain.
  - **Startup Greeting & Hostname**: Set custom welcome greetings and titles.
  - **App Bundles**: Choose whether to bundle the 17-game retro pack, developer emulators (CHIP-8, 6502, BASIC, 3D), and networking tools.
- **Build Bootable Floppy (.dsk)**: One-click builder that generates `images/PlusOS_Floppy.dsk` (1.44M/800K Macintosh HFS floppy) containing only essential boot files and the hard drive installer (zero games).
- **Return to PlusOS Injector**: Configure the target SCSI ID and inject the Classic Mac OS return executable into any disk image or floppy disk image.
- **Open Hard Drive Explorer**: Dedicated button to switch immediately into the Hard Drive Explorer with the built image.

### 2. Hard Drive Explorer & Image Manager
- **Dual-Pane HFS Explorer**: Left pane folder hierarchy tree view and right pane detailed file list (Name, Mac FourCC Type, Creator, Data Size, Resource Fork Size, CNID).
- **Drag-and-Drop File Import**: Drag any file or application from Windows Explorer directly into the Disk Studio window to import it into the current HFS directory with automatic Mac FourCC type and creator detection.
- **File Export & Delete**: Export files from vintage Mac disk images directly to your PC, or remove unwanted items.
- **File Inspection**: Double-click files for instant metadata and content preview.
- **Return to PlusOS App Injection**: Inject the native Classic Mac OS executable (`Return to PlusOS`, `APPL`/`PLOS`) into `:Desktop:Return to PlusOS` on the current image, or into any external hard drive or floppy disk image (`.dsk`). Running this app inside Classic Mac OS rewrites Extended PRAM (XPRAM offset `$78`) and issues the Macintosh Toolbox `_Restart` trap (`$A888`), seamlessly returning the computer to PlusOS.
- **Switch to OS Builder**: Quick button to return to the OS Builder dialogue at any time.

---

## Minimal Bootable Floppy & Hard Drive Installer

PlusOS provides a minimal bootable floppy disk image (`images/PlusOS_Floppy.dsk`) designed for machine recovery, initial hardware provisioning, and minimal footprint boots:
- **Build Command**: `build floppy` or `make floppy` or click **"Build Bootable Floppy (.dsk)"** in Disk Studio.
- **Zero Bloat / No Games**: Contains only the floppy bootloader (`build/boot/loader_floppy.bin`), kernel, system shell, desktop, and essential tools (`FILES`, `CAT`, `LS`, `SYSINFO`, `TASKMAN`, `CALC`, `NOTES`, `MEM`, `VER`, `BOOT`, `REBOOT`, `OFF`, `REG`, `PKG`, `PKGMGR`).
- **Hard Drive Installer (`installer.pex`)**:
  - Bootable directly from the floppy (`:Desktop:Installer` or select "PlusOS Installer" from the boot menu).
  - Automatically probes all SCSI IDs (`0` through `7`) for attached hard drives or BlueSCSI targets.
  - Formats the target drive as Macintosh HFS, writes boot blocks (`loader.bin`), installs the PlusOS kernel, seeds all system binaries, writes `multiboot.cfg`, and injects the Classic Mac OS `Return to PlusOS` application onto the drive.

---

## Package Management & Standalone Repository System

PlusOS includes an **APT / YUM** style binary package distribution system:

### 1. Standalone Package Repository Server (`repository/`)
Completely independent from any website, located in its own dedicated `repository/` folder:
- **Dual HTTP & HTTPS (SSL) Support**:
  - **HTTP (Port `8088`)**: Enables vintage 68000 Mac clients (`pkg`, `pkgmgr`) and local network self-hosters to connect without SSL overhead.
  - **HTTPS (Port `8443`)**: For modern web browsers and secure connections. Automatically generates self-signed certificates or accepts custom certificates (`--cert` / `--key`).
  - **Cloudflare Integration**: Fully compatible with Cloudflare Flexible and Full SSL proxying (`CF-Connecting-IP`, `X-Forwarded-Proto`).
  - **Upstream Mirroring / Proxy Cache**: Supports `--upstream <https-url>` so a local server on your LAN can securely mirror an upstream Cloudflare HTTPS repository and serve it over plain HTTP to vintage Macs.
  - **Git Sync**: Automatic pull/clone support from any Git package repository (`--git <url>` or `POST /api/packages/sync`).
- **Start the Server**:
  - Windows: `repository\run_repo.bat`
  - Linux / macOS / WSL: `cd repository && ./run_repo.sh`

### 2. CLI Package Manager (`pkg`)
Run from the PlusOS command shell:
```sh
pkg update                  # Download & refresh package catalog from repository
pkg list                    # List available and installed packages
pkg search <term>           # Search packages by name or description
pkg install <app>           # Download & install binary (.pex) directly to :bin: or :Games:
pkg remove <app>            # Uninstall package from disk
pkg config repo <host:port> # Point to custom or self-hosted repository (default: 8088)
```

### 3. GUI Software Center (`pkgmgr`)
- Accessible from the desktop menu bar (**PlusOS -> Software Center**) and Start menu.
- Browse packages by category: **[All]**, **[Games]**, **[Dev]**, **[Net]**, **[Tools]**.
- Live installed status indicators (`[INST]`).
- Package details panel: version, category, size in bytes/KB, and description.
- Interactive one-click **[Install / Download]**, **[Launch]**, **[Remove]**, and **[Update Catalog]** buttons.

---

## Screenshots

Captured in Mini vMac by `tools/showcase.ps1` (the full set is in
[`docs/screenshots/`](docs/screenshots/)).

| | |
|---|---|
| ![Desktop](docs/screenshots/22_desktop-clean.png) | ![Programs](docs/screenshots/23_desktop-programs.png) |
| The desktop: Matrix wallpaper, movable icons, menu bar, themes | The Programs launcher (every command, including `net`/`ping`/`telnet`/`irc`) |
| ![Task Manager](docs/screenshots/25_desktop-taskman-gui.png) | ![Files](docs/screenshots/24_desktop-files.png) |
| GUI Task Manager with live CPU bars | The Files explorer |
| ![3D engine](docs/screenshots/20_cube3d-wireframe.png) | ![Networking](docs/screenshots/08_help-all-commands.png) |
| Real-time fixed-point 3D (`cube3d`) | Every built-in command, networking included |

Networking (`net`, `ping`, `telnet`, `irc`) needs a real BlueSCSI — Mini vMac
has no DaynaPORT — so the live online session is shown on hardware, not here.

## Layout

| | |
|---|---|
| `sdks/macplus/` | **Macintosh Plus SDK**: standalone library. `<plusrom/...>` headers, `libplusrom.a`. Programs built with it run on the Plus and the SE. |
| `sdks/macse/` | **Macintosh SE SDK**: standalone library. `<serom/...>` headers, `libserom.a`. Adds the SE-only ROM calls (ADB Manager, KeyTranslate, ...). |
| `boot/loader.S` | boot block: loads the kernel from the disk (assembly: it must fit in 1 KB and runs before anything else exists) |
| `kernel/` | **the kernel**: modular subsystems in `core/` (entry, ROM init), `cpu/` (traps, syscalls, exceptions), `proc/` (multitasking scheduler), `drivers/` (video, input, sound), and `services/` (exec, macloader, window server). |
| `apps/` | **programs categorized by type**: `games/` (13 games), `development/` (emulators, 3D, SDK demos), `tools/` (utilities, task manager), `network/` (TCP/IP clients), and `system/` (desktop, shell, boot). An app folder containing a file named `SE` builds with the Mac SE SDK. |
| `tools/sdkgen/` | generates the two SDKs: assembles the Plus and SE ROMs from the mac_rom sources (checked bit-identical to the originals), decodes their trap tables, writes one header + one C file per ROM manager |
| `tools/` | disk installer, Mini vMac test harness |

Each SDK folder is self-contained (ROM layer, C library, PlusOS runtime,
linker script, Makefile, `sdk.mk`, docs, tests) and can be copied anywhere.
Don't edit the generated SDK folders: change `tools/sdkgen` and run `build sdks`.

## Plus vs. SE

| | Mac Plus | Mac SE |
|---|---|---|
| ROM | 128 KB, version $0075 | 256 KB, version $0276 (SE and SE FDHD) |
| Traps / usable calls | 626 / 641 | 660 / 693 |
| Keyboard | Mac Plus keyboard protocol; PlusOS installs the key translator | ADB; PlusOS reads the KeyMap the ROM's ADB driver maintains |
| Toolbox trap table | $0C00 | $0E00 |

The same disk image boots either machine; the kernel detects which one from
the ROM version.

## How it boots

1. The ROM loads the boot blocks of the HFS volume and runs `loader.S`.
2. The loader reads the kernel image (raw blocks reserved in the HFS bitmap),
   places it just below `BufPtr` and jumps to it.
3. The kernel relocates itself, then starts the ROM's subsystems the way the
   ROM's own boot code would: heaps (`SetApplBase`), event queue
   (`InitEvents`), File Manager + boot volume (`InitFS`/`MountVol`) and QuickDraw.
4. Keyboard: on the Plus it installs its own key translation procedure (the
   ROM leaves that to the System file); on the SE it reads the ADB KeyMap.
   Exception handlers make crashes return to the shell.
5. Boot menu, then the shell. Programs are files on the volume: type a name to run it.

## Shell & Console Features

- **Persistent 20-Command History**: Press **Up / Down Arrow** in the console or Desktop terminal window to recall your last 20 commands. Command history is automatically saved to `:PlusOS History` on the hard drive so it persists across reboots. Run `history` to view or `history -c` to clear.
- **Tab Auto-Completion**: Press **Tab** to auto-complete shell built-ins, programs in `:bin:`, and HFS file/folder paths (`:`, `:Desktop:`, `:Games:`, `:bin:`). Repeated **Tab** presses cycle through matching candidates.
- **Built-ins & Commands**: `help`, `history`, `ls`, `cat`, `files`, `hexedit`, `ps`, `taskman`, `kill`, `mem`, `sysinfo`, `date`, `ver`, `reg`, `sound`, `clip`, `calc`, `notes`, `puzzle`, `net`, `ping`, `telnet`, `irc`, `macrun`, `chip8`, `emu6502`, `basic`, `cube3d`, `desktop`, `startup`, `boot`, `reboot`, `off`, or the name of any `.pex` or Classic Mac `'APPL'` program on disk.

## Networking (BlueSCSI WiFi / DaynaPORT)

PlusOS gets on the Internet through a **BlueSCSI V2**'s built-in WiFi, which
emulates a **DaynaPORT SCSI/Link** Ethernet adapter on the SCSI bus. PlusNET
(`<plusnet/net.h>`, `libplusnet`) is a from-scratch TCP/IP stack: a DaynaPORT
SCSI driver, ARP, IPv4, ICMP, UDP, a DHCP client, a DNS resolver, and TCP.

```
net                     bring the adapter up (DHCP by default); show IP, MAC, gateway, DNS
net ip 192.168.1.50 255.255.255.0 192.168.1.1 1.1.1.1   use a static address instead
net id 4                SCSI ID of the DaynaPORT (BlueSCSI's empty NEx.hda)
net stats               frame counters and the last SCSI error
ping google.com         ICMP echo
telnet towel.blinkenlights.nl
irc irc.libera.chat mynick #channel
```

Enable WiFi in `bluescsi.ini` (`WiFiSSID` / `WiFipassword`) and give the card an
empty network device file (e.g. `NE4.hda` at SCSI ID 4). Networking needs the
real hardware — Mini vMac has no DaynaPORT, so `net` there just reports
"no device".

## Highlights

- **Classic Macintosh `'APPL'` Executable Loader (`kernel/macloader.c`, `macrun`)**: Parses HFS Resource Forks, `.rsrc`, and MacBinary `.bin` containers, constructs the Macintosh A5 World and Jump Table from `'CODE'` 0, hooks `_LoadSeg` (`$A9F0`), `_UnLoadSeg` (`$A9F1`), `_GetAppParms` (`$A9F5`), and `_ExitToShell` (`$A9F4`), and runs Classic 68000 Mac applications.
- **Tear-Free Double-Buffered Video & 2D/3D Polygons/Hexagons (`<plusos/gfx.h>`, `<plus3d/plus3d.h>`, `cube3d`)**: Unrolled 64-byte scanline VBL-synced page flipping (`vblit.S`), 16-shade 4x4 ordered Bayer dithering, 2D polygons/N-gons/hexagons (`gfx_poly_fill`, `gfx_poly_dither`, `gfx_ngon_fill`, `gfx_hexagon`), and 3D meshes (`mesh_build_cube`, `mesh_build_hex_prism`, `mesh_build_pyramid`, `mesh_build_octahedron`, `mesh_build_torus`).
- **System 7.1 Features & Desk Accessories**: 22.25 kHz 4-voice PWM hardware sound synthesizer (`sound`), persistent Scrap/Clipboard Manager (`clip`), Calculator (`calc`), Note Pad (`notes`), and 15-Puzzle (`puzzle`).
- **Internet over BlueSCSI WiFi (`<plusnet/net.h>`, `net` / `ping` / `telnet` / `irc`)**: a from-scratch TCP/IP stack — DaynaPORT SCSI/Link driver, ARP, IPv4, ICMP, UDP, DHCP client, DNS resolver and TCP — driving the ROM SCSI Manager (`_SCSIDispatch`) directly. telnet and IRC clients included.
- **Fixed-point 3D engine (`<plus3d/plus3d.h>`, `cube3d`)**: the whole pipeline is 16.16 integer math on the FPU-less 68000 — hardware `MULS.W`/`DIVS.W`, a sine table, and a direct-to-framebuffer rasterizer — so the solid/wireframe models spin in real time.
- **Emulators**: CHIP-8 Arcade VM (`chip8`), MOS 6502 / Apple I Woz Monitor (`emu6502`), and Retro BASIC with polygon/hexagon graphics (`basic`).

See [`docs/MAC_EXEC_AND_RENDERING.md`](docs/MAC_EXEC_AND_RENDERING.md) for the complete technical architecture and documentation.

## Community & Discord

Have questions, want to request features, discuss vintage Mac hardware, or just want to hang out? Come join the community on Discord:

👉 **[Join the PlusOS Discord Server](https://discord.gg/hkpe76G7AU)** (https://discord.gg/hkpe76G7AU)


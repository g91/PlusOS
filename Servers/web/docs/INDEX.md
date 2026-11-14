# PlusOS Technical Documentation Suite

> **PlusOS: Bare-Metal 68000 Operating System for Macintosh Plus & Macintosh SE**  
> **Source Repository:** `C:\Users\Evil\PlusOS`  
> **Upstream ROM Assembly Sources:** [https://github.com/jonathanschilling/mac_rom](https://github.com/jonathanschilling/mac_rom)  
> **Documentation Root:** `C:\Users\Evil\PlusOS\docs\`  

---

## Upstream ROM Assembly & Universal Portable Toolchain

* **ROM Assembly Source Repository**: The bit-identical 68000 assembly source tree for all classic Macintosh ROMs is hosted at **[https://github.com/jonathanschilling/mac_rom](https://github.com/jonathanschilling/mac_rom)**. It can be cloned locally into `PlusOS/mac_rom/` and compiled using standard open GNU cross-assemblers (`binutils-m68k-linux-gnu` / `m68k-elf-binutils`).
* **Universal & Portable Build Pipeline**: PlusOS build files (`config.mk`, `Makefile`, `build.bat`, `vmac.ps1`) are fully self-contained. The build system automatically checks in-tree paths (`mac_rom/`, `images/`), auto-detects cross-compilers from PATH, and operates independently without requiring an external hard drive or SD card (`F:\`).

---

## Master Documentation Directory

This documentation suite provides complete technical documentation, architecture deep-dives, developer playbooks, and hardware references for PlusOS and the Macintosh ROM.

> Every document below is a plain **Markdown (`.md`) file** living in the `docs/` folder. Each link opens it right here on the site; you can also read any of them offline by opening the matching `.md` file in the `docs/` folder with any text editor or Markdown viewer.

| Document (`.md` file) | Title | Description |
|:---|:---|:---|
| **[01_SYSTEM_FEATURES_AND_SUBSYSTEMS.md](/docs/01_SYSTEM_FEATURES_AND_SUBSYSTEMS.md)** | **Operating System Features & Architecture** | 3 Software Rings (Ring 0 Kernel, Ring 1 Services, Ring 2 Userland), 68000 CPU privilege enforcement, Syscall gate (`TRAP #1`), preemptive multitasking scheduler (`struct proc`), virtual console with 20-command persistent history, tab auto-completion, window server (`winsrv`), graphical desktop with matrix background and theme engine, `.pex` native binaries, Classic Mac `'APPL'` compatibility layer (`macloader.c`), PlusNET TCP/IP stack over BlueSCSI WiFi, 4-voice PWM sound engine, and system registry. |
| **[02_HOW_TO_MAKE_A_BOOT.md](/docs/02_HOW_TO_MAKE_A_BOOT.md)** | **How to Make a Bootable System & Complete Boot Pipeline** | End-to-end boot sequence from power-on reset vectors ($0000/$0004) to userland shell/desktop. Anatomy of the 1 KB Boot Block (`boot/loader.S`, `$4C4B` "LK" magic, version `$44xx`), finding the boot drive in the ROM drive queue (`$030A`), reading raw kernel blocks via ROM driver `_Read`, kernel relocation, ROM subsystem initialization (`SetApplBase`, `InitEvents`, `InitFS`, `MountVol`, `InitGraf`), keyboard setup (Plus vs SE ADB), and step-by-step tutorial on creating bootable disk images with `tools/install.py` for BlueSCSI and Mini vMac. |
| **[03_HOW_TO_GENERATE_VIDEO.md](/docs/03_HOW_TO_GENERATE_VIDEO.md)** | **Video Generation, Framebuffers & 2D/3D Rendering** | Hardware video rasterization on compact Macs (512×342 1-bit monochrome CRT, `ScrnBase` at `$0824`, 64 bytes/scanline, BBU/PAL DMA interleaving, 60.15 Hz VBL). PlusOS tear-free double-buffered pipeline (`kernel/video.c`), assembly scanline blitter (`vblit.S`), 16-shade 4×4 ordered Bayer dithering, scanline polygon and hexagon filling (`<plusos/gfx.h>`), Plus3D 16.16 fixed-point 3D projection engine (`<plus3d/plus3d.h>`), and modular Mac framebuffer reference (DAFB, Civic, RAMDACs, CLUTs). |
| **[04_SYSTEM_OPERATIONS_PLAYBOOK.md](/docs/04_SYSTEM_OPERATIONS_PLAYBOOK.md)** | **System Operations Playbook & Developer Cookbook** | Hands-on developer guide with compilable C, C++, and Assembly code examples: memory management (PlusOS API vs ROM `NewPtr`/`NewHandle`), text console I/O, reading and writing files via HFS (`FSOpen`, `FSRead`, `FSClose`), 2D graphics animation with PlusGFX, real-time 3D rendering with Plus3D, desktop window creation, TCP/IP networking with PlusNET (DHCP, Ping, TCP sockets), hardware audio synthesis, and launching child processes and Classic Mac applications. |
| **[05_HOW_TO_ACCESS_THE_ROM.md](/docs/05_HOW_TO_ACCESS_THE_ROM.md)** | **How to Access the ROM, Traps & Low Memory** | Physical memory mapping of ROM at `$00400000`, low-memory globals (`ROMBase`, `SysZone`, `OSTable`, `ToolTable`), Line 1010 (EMT 1010) A-line trap mechanism ($A000 OS traps vs $A800 Toolbox traps). PlusOS A-line trap gate (`kernel/aline.S`): solving the Ring 2 user mode dilemma, dynamic trampoline construction, stack pointer bridging (`USP` vs `SSP`), patching ROM traps dynamically (`GetTrapAddress`, `SetTrapAddress`), and accessing ROM resources via `RomMapInsert`. |
| **[06_HOW_TO_USE_THE_SDK_TO_ACCESS_ROM.md](/docs/06_HOW_TO_USE_THE_SDK_TO_ACCESS_ROM.md)** | **How to Use the SDK to Access the ROM** | Architecture of `sdks/macplus/` and `sdks/macse/`, header structure (`<plusrom/...>`, `<serom/...>`, `<plusos/...>`, `<libc/...>`), how inline traps are called from C via `#pragma parameter` and `M68K_INLINE` opcode injection, compiler flags (`-m68000`, `-ffixed-a5`, `-fPIE`), linker scripts (`app.ld`), and complete runnable C and C++ applications invoking ROM traps. |
| **[07_ROM_ENCYCLOPEDIA_AND_INTERNALS.md](/docs/07_ROM_ENCYCLOPEDIA_AND_INTERNALS.md)** | **ROM Encyclopedia, Internals & C++ Reimplementation** | Complete comparison of target ROMs (128 KB Mac Plus vs 256 KB Mac SE / SE FDHD), ROM header anatomy ($0000–$0038), 32-bit big-endian additive checksums, SE dual 128KB chip split checksums, fixing modified ROMs with `tools/fixup_checksum.py`, PlusOS tweaked SE ROM with 5-second boot search optimization, PlusOS C++ reimplementation scaffold (`ROM/SE/cpp/`, 102 modules, 9,545 entry points), and hidden Easter eggs (steganographic team photos). |

---

## Supplementary Architecture Documents

* **[ARCHITECTURE.md](/docs/ARCHITECTURE.md)**: Original high-level architectural roadmap and ring division specification.
* **[MAC_EXEC_AND_RENDERING.md](/docs/MAC_EXEC_AND_RENDERING.md)**: Classic Mac `'APPL'` binary loader internals, A5 World construction, Jump Table parsing, and Bayer dithering math.

# PlusOS

A bare-metal operating system for the Macintosh Plus and Macintosh SE,
written in C on top of the Mac ROM, plus one SDK per machine.

Source lives on Windows; the m68k cross compiler runs in WSL (Ubuntu).
`build.bat` runs everything from a Windows prompt.

```
build              build both SDK libraries, boot loader, kernel, apps
build install      write it all to the BlueSCSI disk images on the SD card (F:)
build test         install, then boot it in Mini vMac
build sdks         regenerate both SDK folders from the Plus and SE ROMs
build sdktest      run both SDKs' self-tests
build clean
```

## Layout

| | |
|---|---|
| `sdks/macplus/` | **Macintosh Plus SDK**: standalone library. `<plusrom/...>` headers, `libplusrom.a`. Programs built with it run on the Plus and the SE. |
| `sdks/macse/` | **Macintosh SE SDK**: standalone library. `<serom/...>` headers, `libserom.a`. Adds the SE-only ROM calls (ADB Manager, KeyTranslate, ...). |
| `boot/loader.S` | boot block: loads the kernel from the disk (assembly: it must fit in 1 KB and runs before anything else exists) |
| `kernel/` | the kernel: ROM start-up, console, keyboard, program loader, shell, boot menu. One kernel for both machines. |
| `apps/` | example programs: `hello` (C), `qdemo` (C++ / QuickDraw). An app folder containing a file named `SE` builds with the Mac SE SDK. |
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
- **Built-ins & Commands**: `help`, `history`, `ls`, `cat`, `files`, `hexedit`, `ps`, `taskman`, `kill`, `mem`, `sysinfo`, `date`, `ver`, `reg`, `sound`, `clip`, `calc`, `notes`, `puzzle`, `macrun`, `chip8`, `emu6502`, `basic`, `cube3d`, `desktop`, `startup`, `boot`, `reboot`, `off`, or the name of any `.pex` or Classic Mac `'APPL'` program on disk.

## Highlights

- **Classic Macintosh `'APPL'` Executable Loader (`kernel/macloader.c`, `macrun`)**: Parses HFS Resource Forks, `.rsrc`, and MacBinary `.bin` containers, constructs the Macintosh A5 World and Jump Table from `'CODE'` 0, hooks `_LoadSeg` (`$A9F0`), `_UnLoadSeg` (`$A9F1`), `_GetAppParms` (`$A9F5`), and `_ExitToShell` (`$A9F4`), and runs Classic 68000 Mac applications.
- **Tear-Free Double-Buffered Video & 2D/3D Polygons/Hexagons (`<plusos/gfx.h>`, `<plus3d/plus3d.h>`, `cube3d`)**: Unrolled 64-byte scanline VBL-synced page flipping (`vblit.S`), 16-shade 4x4 ordered Bayer dithering, 2D polygons/N-gons/hexagons (`gfx_poly_fill`, `gfx_poly_dither`, `gfx_ngon_fill`, `gfx_hexagon`), and 3D meshes (`mesh_build_cube`, `mesh_build_hex_prism`, `mesh_build_pyramid`, `mesh_build_octahedron`, `mesh_build_torus`).
- **System 7.1 Features & Desk Accessories**: 22.25 kHz 4-voice PWM hardware sound synthesizer (`sound`), VBL stack sniffer, persistent Scrap/Clipboard Manager (`clip`), Calculator (`calc`), Note Pad (`notes`), and 15-Puzzle (`puzzle`).
- **Emulators**: CHIP-8 Arcade VM (`chip8`), MOS 6502 / Apple I Woz Monitor (`emu6502`), and Retro BASIC with polygon/hexagon graphics (`basic`).

See [`docs/MAC_EXEC_AND_RENDERING.md`](docs/MAC_EXEC_AND_RENDERING.md) for the complete technical architecture and documentation.

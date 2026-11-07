# Macintosh SE ROM — C++ scaffold (`libserom_cpp.a`)

A C++ reimplementation **scaffold** mirroring every module of the Macintosh SE
ROM, compiled for the 68000 with the PlusOS Retro68 toolchain
(`m68k-apple-macos-g++` 16.1).

## What this is

- **102 modules**, one C++ header + source per `se/*.s` assembly module, laid
  out in the same tree (`boot/`, `hw/`, `os/`, `toolbox/`, `toolbox/quickdraw/`,
  `debugger/`, `resources/`, `extra/`).
- **9,545 entry points** — every top-level label in the assembly source is
  declared and defined as a C++ function, inside a namespace that mirrors the
  module path: `se/boot/boot_search.s` → `serom::boot::boot_search::…`.
- Builds into **`libserom_cpp.a`** (elf32-m68k, m68k:68000).

## What this is NOT

This is **complete in surface, not in behavior**. Every routine is a stub whose
body calls `serom::serom_unimplemented(...)`; none of the 68000 logic has been
ported. It is a structural starting point for a C++ port, **not** a functioning
ROM and not a drop-in replacement for `../MacSE.ROM`. The real, working ROM is
the assembled binary one directory up (`../MacSE.ROM`), built from the
hand-written 68000 source at `F:\mac_rom`.

Porting a module means replacing its stub bodies with real C++ and removing the
`serom_unimplemented` calls, one module at a time.

## Layout

```
include/serom/<path>.hpp   per-module declarations (namespace serom::<dirs>::<file>)
include/serom/all.hpp      umbrella header (includes every module)
include/serom/support.hpp  serom_unimplemented() declaration
src/<path>.cpp             per-module stub definitions
src/support.cpp            serom_unimplemented() definition
Makefile                   builds libserom_cpp.a with Retro68
```

## Build (inside WSL Ubuntu)

```sh
cd /mnt/c/Users/Evil/PlusOS/ROM/SE/cpp
make            # -> libserom_cpp.a
make check      # syntax-check the umbrella header (all modules together)
make clean
```

Override the toolchain path with `make TC=/path/to/retro68/bin` if needed.

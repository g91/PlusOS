# PlusOS SDK for the Macintosh Plus

A standalone C/C++ SDK for programs that run under PlusOS on the
Macintosh Plus (68000). It is a self-contained library: copy this folder
anywhere, build it once, and point your program's Makefile at it.

There are two PlusOS SDKs, one per machine, each in its own folder:

| SDK | Folder | ROM headers | ROM library |
|---|---|---|---|
| Macintosh Plus | `sdks/macplus` | `#include <plusrom/...>` | `libplusrom.a` |
| Macintosh SE | `sdks/macse` | `#include <serom/...>` | `libserom.a` |

Programs built with the **Mac Plus SDK also run on the Mac SE** (the SE ROM
contains every Plus trap). Use the Mac SE SDK when you need SE-only ROM calls
such as the ADB Manager.

## What's in it

| Part | Where | What |
|---|---|---|
| ROM layer | `include/plusrom/<Manager>.h` | One header per ROM manager (`QuickDraw.h`, `MemoryMgr.h`, `FileMgr.h`, `EventMgr.h`, ...: 68 in all), each with its own declarations, including only the managers it depends on. |
| | `src/plusrom/<Manager>.c` | One C file per manager: a run-time catalog of its calls (trap, selector, in ROM or not, ROM routine offset), plus glue. Built into `libplusrom.a`. |
| | `traps.h` | All 626 traps the Macintosh Plus ROM implements (482 Toolbox + 144 OS), decoded from the ROM's own dispatch table, plus run-time helpers (`plusrom_machine()`, `plusrom_trap_address()`, ...). |
| | `lowmem.h`, `hw.h`, `equates.h` | Low-memory globals (382), hardware registers (74) and ROM equates (1580) from the ROM source. |
| | `boot.h` | Boot-time OS calls that were never in the public interfaces (`InitEvents`, `InitFS`, `InitZone`, `GetOSEvent`, ...). |
| | `catalog.h` | Look up any call at run time: `plusrom_find_call("FrameRect", &mgr)`. |
| Runtime | `include/plusos/` | The PlusOS kernel service table (`api.h`), executable format (`exe.h`), convenience wrappers (`plusos.h`). |
| C library | `include/libc/` | Freestanding `string.h`, `stdio.h` (printf family), `stdlib.h` (malloc on the ROM heap), `ctype.h`, `setjmp.h`, `assert.h`. |
| Docs | `docs/ROM_REFERENCE.md` | Every call by manager, and every trap in the ROM with its routine and offset. |

Calls the Macintosh Plus ROM does not have are still declared but marked
unavailable, so using one is a compile error that says why:

```
error: 'ADBReInit' is unavailable: ADBReInit: trap 0xA07B is not in the Mac Plus ROM
```

## Build

```
make            # build/lib: libc.a libplusrom.a libplusos.a librt_app.a crt0_app.o crt0_kernel.o
make test       # compile-check every header in C and C++
```

## Writing a program

```c
#include <plusos/plusos.h>          // printf, getchar, malloc, ...
#include <plusrom/QuickDraw.h>     // any ROM manager you need

int main(int argc, char **argv)
{
    printf("Hello from the Macintosh Plus!\n");
    Rect r = { 20, 20, 120, 220 };
    PaintOval(&r);
    return 0;
}
```

Makefile:

```make
PLUSOS_SDK := /path/to/sdks/macplus
include $(PLUSOS_SDK)/sdk.mk

hello.pex: hello.c.o
	$(call plusos_link_app,$@,$^,HELLO)

%.c.o: %.c
	$(CC) $(PLUSOS_CPPFLAGS) $(PLUSOS_CFLAGS) -c $< -o $@
```

C++ works too (classes, virtual functions, `new`/`delete`, global
constructors); build with `$(PLUSOS_CXXFLAGS)` (no exceptions or RTTI).

Programs are `.pex` files, linked at address 0 with a relocation table so the
kernel can load them anywhere. Copy one onto the PlusOS disk and type its
name at the shell prompt.

## Requirements

The Retro68 cross compiler (`m68k-apple-macos-gcc`; set `TOOLCHAIN` in
`sdk.mk`) and Python 3.

The ROM layer is generated from the Macintosh Plus ROM by PlusOS's `tools/sdkgen`.
Regenerate both SDKs from the PlusOS folder with `build sdks`.

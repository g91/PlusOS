# SDK & Application Development Guide

PlusOS provides two standalone SDKs:
1. **Macintosh Plus SDK (`sdks/macplus/`)**: Generates code compatible with both the Macintosh Plus and Macintosh SE.
2. **Macintosh SE SDK (`sdks/macse/`)**: Unlocks SE-specific ROM traps (ADB Manager, KeyTranslate, expanded SCSI).

---

## 1. SDK Anatomy

Each SDK folder is completely self-contained:
```text
sdks/macplus/
├── include/
│   ├── plusos/         # Core PlusOS API, syscalls, threads, gfx, themes
│   ├── plusnet/        # TCP/IP sockets, DNS, DaynaPORT networking
│   ├── plus3d/         # 16.16 fixed-point 3D polygon/mesh engine
│   └── plusrom/        # Complete Macintosh Plus ROM C headers (QuickDraw, FileMgr, etc.)
├── lib/
│   ├── libplusos.a     # PlusOS runtime & system-call wrappers
│   ├── libplusnet.a    # TCP/IP network library
│   ├── libplus3d.a     # Fixed-point 3D math & rasterization library
│   └── libplusrom.a    # ROM trap stubs and manager bindings
├── app.ld              # Linker script (relocatable userland binaries at address 0)
└── sdk.mk              # Make include file for application builds
```

---

## 2. Writing Your First PlusOS Program

Create a source file `hello.c`:

```c
#include <plusos/plusos.h>
#include <plusos/gfx.h>
#include <plusrom/QuickDraw.h>

int main(int argc, char **argv)
{
    printf("Hello from PlusOS 3.1 by the1Domo!\n");
    if (argc > 1) {
        printf("Arguments received:\n");
        for (int i = 1; i < argc; i++) {
            printf("  [%d]: %s\n", i, argv[i]);
        }
    }
    
    /* Beep the speaker using ROM SysBeep */
    SysBeep(10);
    return 0;
}
```

---

## 3. Creating a GUI Windowed Application

PlusOS makes creating interactive desktop applications straightforward:

```c
#include <plusos/plusos.h>
#include <plusos/syscall.h>
#include <plusos/gfx.h>

static void draw_window(GSurface *s)
{
    /* Clear surface to white */
    gfx_fill_surface(s, 0);
    
    /* Draw frame and title */
    gfx_rect(s, 4, 4, s->w - 4, s->h - 4, 1);
    gfx_text_surface(s, 16, 20, "My PlusOS App", 0);
    
    /* Draw 4x4 dithered rectangle */
    gfx_dither_rect(s, 16, 36, 120, 80, 8);
}

int main(void)
{
    /* Request a 200x120 window from the window server */
    struct plusos_win_args w = {
        .op = PLUSOS_WIN_CREATE,
        .title = "Hello Window",
        .w = 200,
        .h = 120,
    };
    int wid = (int)plusos_syscall(SYS_win, &w);
    if (wid < 0) return 1;

    /* Draw window content */
    draw_window(&w.surface);
    
    /* Notify window server to composite to screen */
    w.op = PLUSOS_WIN_UPDATE;
    plusos_syscall(SYS_win, &w);

    /* Event loop */
    for (;;) {
        struct plusos_event e;
        w.op = PLUSOS_WIN_GETEVENT;
        w.ev = &e;
        if (plusos_syscall(SYS_win, &w) > 0) {
            if (e.type == PLUSOS_EV_CLOSE) break;
        }
        plusos_syscall(SYS_sleep, (void*)2);
    }

    w.op = PLUSOS_WIN_DESTROY;
    plusos_syscall(SYS_win, &w);
    return 0;
}
```

---

## 4. Compiling Applications

Applications are compiled with `m68k-elf-gcc` using the position-independent relocation pipeline:

```bash
# Compile and link with SDK
m68k-elf-gcc -m68000 -O2 -ffreestanding -fno-builtin \
    -I../../sdks/macplus/include \
    -T../../sdks/macplus/app.ld \
    -Wl,--emit-relocs -o hello.elf hello.c \
    -L../../sdks/macplus/lib -lplusos -lplusrom

# Generate relocatable .pex binary
python3 ../../tools/mkreloc.py hello.elf hello.pex
```

The resulting `.pex` file can be copied directly to `:bin:` on the `PlusOS.hda` disk image!

---

## Navigation

- **Previous Chapter**: [← 11. Complete Syscall ABI Reference (SYS 1-63)](/wiki/11-syscall-abi-reference)
- **Next Chapter**: [13. PlusOS Disk Studio & Compiler Pipeline →](/wiki/13-plusos-disk-studio-and-compiler)
- **Graphical Apps**: [22. PlusUI: GUI Framework, Designer & Code Generator →](/wiki/22-plusui-gui-framework-and-designer)
- **Table of Contents**: [Return to Getting Started](/wiki/01-getting-started)

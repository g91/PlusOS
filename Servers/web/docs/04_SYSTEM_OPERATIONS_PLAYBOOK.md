# PlusOS: System Operations Playbook & Developer Cookbook

> **Operating System:** PlusOS  
> **Source SDKs:** `sdks/macplus/` and `sdks/macse/`  
> **Target Environment:** C / C++ / Assembly on Motorola 68000  

---

## 1. Developer Environment & Application Anatomy

Every user program in PlusOS is an executable binary (usually compiled as a self-relocating `.pex` file) running in **68000 User Mode** (`SR = $0000`).

### Application Directory Structure
Applications live in `C:\Users\Evil\PlusOS\apps\<name>/`:
```
apps/
  └── myapp/
        ├── myapp.c         # Main C source file
        ├── Makefile        # Uses include ../../sdks/macplus/sdk.mk
        └── SE              # (Optional) If present, builds with Mac SE SDK instead of Mac Plus SDK
```

### Minimal Makefile (`apps/myapp/Makefile`)
```makefile
TARGET   = myapp
OBJS     = myapp.o
SDK_DIR ?= ../../sdks/macplus

include $(SDK_DIR)/sdk.mk
```

### Standard Program Template (`apps/myapp/myapp.c`)
```c
#include <plusos/plusos.h>
#include <stdio.h>

int main(int argc, char **argv)
{
    printf("Hello from PlusOS! Running on %s\n", plusrom_machine_name());
    return 0;
}
```

---

## 2. Memory Allocation & Management

PlusOS applications have three distinct ways to manage dynamic memory:

### Option A: Standard C Library (`malloc` / `free`)
PlusOS SDK provides a full `libc.a` built on top of the kernel's memory allocator:
```c
#include <stdlib.h>

char *buffer = (char *)malloc(1024);
if (!buffer) {
    printf("Out of memory!\n");
    return -1;
}
memset(buffer, 0, 1024);
free(buffer);
```

### Option B: PlusOS Kernel Service Table (`plusos->mem_*`)
Directly allocate from the unified application memory pool:
```c
void *ptr = plusos->mem_alloc(4096);
uint32_t free_ram = plusos->mem_free_bytes();
printf("Allocated 4KB. Free RAM: %lu KB\n", free_ram >> 10);
plusos->mem_free(ptr);
```

### Option C: Macintosh ROM Memory Manager Traps
Applications can invoke the ROM Memory Manager directly via `<plusrom/MemoryMgr.h>`:
```c
#include <plusrom/MemoryMgr.h>

/* Allocate non-relocatable pointer */
Ptr p = NewPtr(2048);
if (MemError() == noErr) {
    DisposPtr(p);
}

/* Allocate relocatable handle (double indirection) */
Handle h = NewHandle(8192);
HLock(h);                   /* Lock handle before dereferencing */
char *data = *h;
/* ... use data ... */
HUnlock(h);                 /* Unlock to allow heap compaction */
DisposHandle(h);
```

---

## 3. Console & Virtual Terminal I/O

The virtual console provides 64 columns by 42 rows of formatted text with screen positioning and inverted styles:

```c
#include <plusos/plusos.h>
#include <stdio.h>

void console_demo(void)
{
    /* Clear text screen */
    con_clear();

    /* Position cursor at column 20, row 5 */
    con_goto(20, 5);

    /* Print inverted text (white-on-black) */
    con_invert(1);
    printf(" *** SYSTEM CONTROL PANEL *** ");
    con_invert(0);

    /* Move cursor */
    con_goto(10, 8);
    printf("Type your name: ");

    /* Interactive line editor with backspace support */
    char name[32];
    int len = plusos->key_line(name, sizeof(name));
    
    con_goto(10, 10);
    printf("Welcome, %s! (Length: %d)\n", name, len);

    /* Non-blocking key poll */
    printf("Press any key to continue...\n");
    while (!plusos->key_poll()) {
        plusos->sleep_ticks(1);
    }
}
```

---

## 4. File I/O on the Boot HFS Volume

PlusOS mounts the boot volume during kernel startup and exposes its volume reference number via `plusos->boot_vrefnum`:

```c
#include <plusos/plusos.h>
#include <plusrom/FileMgr.h>

/* Read an entire file into memory */
int read_file_demo(const char *filename)
{
    short refnum;
    OSErr err;

    /* Convert C string to Pascal string (length byte prefix) */
    unsigned char pstr[64];
    c2pstrcpy(pstr, filename);

    /* Open the Data Fork of the file */
    err = FSOpen(pstr, plusos->boot_vrefnum, &refnum);
    if (err != noErr) {
        printf("Cannot open file %s (error %d)\n", filename, err);
        return -1;
    }

    /* Get file size */
    long size;
    GetEOF(refnum, &size);
    printf("Reading %s (%ld bytes)...\n", filename, size);

    /* Allocate buffer and read */
    char *buf = (char *)malloc(size + 1);
    long count = size;
    err = FSRead(refnum, &count, buf);
    buf[count] = '\0';

    FSClose(refnum);

    printf("File contents:\n%s\n", buf);
    free(buf);
    return 0;
}
```

---

## 5. 2D Graphics & Double-Buffered Animation (`PlusGFX`)

Build high-framerate, tear-free graphics using `libplusgfx.a`:

```c
#include <plusos/plusos.h>
#include <plusos/gfx.h>

void bouncing_ball_demo(void)
{
    /* Initialize drawing target to the offscreen back buffer */
    GSurface *back = gfx_backbuffer();
    gfx_target(back);

    int x = 100, y = 100;
    int dx = 2, dy = 2;
    int radius = 20;

    for (int frame = 0; frame < 300; frame++) {
        /* 1. Clear frame to white */
        gfx_fill(0, 0, GFX_SCREEN_W, GFX_SCREEN_H, 0);

        /* 2. Draw 50% desktop gray frame border */
        gfx_gray(10, 10, GFX_SCREEN_W - 10, GFX_SCREEN_H - 10);
        gfx_fill(20, 20, GFX_SCREEN_W - 20, GFX_SCREEN_H - 20, 0);

        /* 3. Draw a dithered hexagon and a solid bouncing circle */
        gfx_hexagon(256, 171, 60, frame * 2, 8, 1);
        gfx_fill_circle(x, y, radius, 1);

        /* 4. Draw HUD text */
        gfx_text(30, 30, "PlusGFX 60 FPS Bouncing Animation", 0);

        /* 5. Flip frame buffer to live CRT screen in one tear-free pass */
        gfx_present();

        /* Update physics */
        x += dx; y += dy;
        if (x - radius < 20 || x + radius > GFX_SCREEN_W - 20) dx = -dx;
        if (y - radius < 20 || y + radius > GFX_SCREEN_H - 20) dy = -dy;

        /* Exit on key press */
        if (plusos->key_poll()) break;
    }
}
```

---

## 6. Real-Time 3D Graphics (`Plus3D`)

Render wireframe and shaded 3D objects at 60 FPS using 16.16 fixed-point math:

```c
#include <plusos/plusos.h>
#include <plusos/gfx.h>
#include <plus3d/plus3d.h>

void cube3d_demo(void)
{
    Mesh3D mesh;
    mesh_build_cube(&mesh, 50);     /* Build 50-pixel cube */

    GSurface *back = gfx_backbuffer();
    gfx_target(back);

    Fixed32 angle_x = 0, angle_y = 0;

    while (!plusos->key_poll()) {
        gfx_fill(0, 0, GFX_SCREEN_W, GFX_SCREEN_H, 0);

        /* Rotate mesh around X and Y axes */
        Matrix3D mat;
        mat3d_identity(&mat);
        mat3d_rotate_x(&mat, angle_x);
        mat3d_rotate_y(&mat, angle_y);

        /* Project and render wireframe */
        mesh_render_wireframe(&mesh, &mat, 256, 171, 300);

        gfx_present();

        angle_x += 0x00020000;      /* Fixed-point increment */
        angle_y += 0x00030000;
    }
}
```

---

## 7. Networking with PlusNET (TCP/IP)

Bring up the DaynaPORT SCSI network interface and connect to Internet servers:

```c
#include <plusos/plusos.h>
#include <plusnet/net.h>

void network_demo(void)
{
    /* 1. Initialize DaynaPORT SCSI/Link adapter */
    printf("Initializing network adapter at SCSI ID 4...\n");
    if (net_init(4) != 0) {
        printf("No DaynaPORT found.\n");
        return;
    }

    /* 2. Obtain IP address via DHCP */
    printf("Acquiring IP address via DHCP...\n");
    if (dhcp_bind() != 0) {
        printf("DHCP failed.\n");
        return;
    }
    printf("Network Online! IP: %s\n", net_ntoa(net_get_ip()));

    /* 3. Send ICMP Ping */
    uint32_t target_ip = dns_resolve("1.1.1.1");
    printf("Pinging 1.1.1.1...\n");
    int rtt = ping_send(target_ip, 1000);
    if (rtt >= 0) {
        printf("Reply from 1.1.1.1: time=%d ms\n", rtt);
    } else {
        printf("Request timed out.\n");
    }
}
```

---

## 8. Playing Hardware Audio & Synthesizer Tones

Generate sound through the Macintosh sound chip:

```c
#include <plusos/plusos.h>

void play_alert_chime(void)
{
    /* Play 440 Hz tone (A4) for 15 ticks (1/4 second) */
    sound_play_tone(440, 15);
    plusos->sleep_ticks(15);

    /* Play 880 Hz tone (A5) for 30 ticks (1/2 second) */
    sound_play_tone(880, 30);
}
```

---

## 9. Launching Executables & Classic Mac Software

PlusOS programs can spawn other programs or execute classic Mac binaries:

```c
#include <plusos/plusos.h>

void launch_examples(void)
{
    /* Launch native PlusOS program */
    char *args[] = { "cube3d", 0 };
    plusos->run("cube3d", 1, args);

    /* Launch Classic Macintosh application */
    char *mac_args[] = { "macrun", ":Apps:MacPaint", 0 };
    plusos->run("macrun", 2, mac_args);
}
```

# PlusOS: Video Generation, Framebuffers, 2D/3D Rendering & Display Hardware

> **Operating System:** PlusOS  
> **Source Modules:** `kernel/video.c`, `kernel/vblit.S`, `<plusos/gfx.h>`, `<plus3d/plus3d.h>`, `apps/cube3d/cube3d.c`  

> **Target Hardware:** Macintosh Plus & SE 1-bit CRT (512×342), with modular Mac framebuffer reference  

---

## 1. Physical Hardware Video Generation on Compact Macs

The classic Macintosh (128K, 512K, Plus, SE) does not have a dedicated GPU or hardware graphics coprocessor. Video generation is performed directly from main system RAM by discrete board timing circuitry:

```
+-------------------------------------------------------------------------+
|                  Motorola 68000 CPU Memory Bus (16-bit)                 |
+-------------------------------------------------------------------------+
       |                                                    |
       v (CPU Cycles)                                       v (DMA Readout)
+-------------------------------+                  +----------------------+
| Main RAM (System & App Heaps) |                  | Video Frame Buffer   |
| $000000 - $3FA6FF (on 4MB)    |                  | $3FA700 - $3FFCFF    |
+-------------------------------+                  +----------------------+
                                                            |
                                                            v 16-bit words
                                                   +----------------------+
                                                   | Video Shift Register |
                                                   | (BBU / PAL Logic)    |
                                                   +----------------------+
                                                            |
                                                            v Serial Bitstream
                                                   +----------------------+
                                                   | Analog Board CRT     |
                                                   | Video Beam (512x342) |
                                                   +----------------------+
```

### Video Timing & Raster Parameters
* **Pixel Resolution**: 512 pixels wide by 342 scanlines high.
* **Pixel Aspect Ratio**: 1:1 square pixels (72 pixels per inch).
* **Pixel Format**: 1 bit per pixel (1 bpp).
  * `0` = White pixel (beam off).
  * `1` = Black pixel (beam full on).
  * Most significant bit (`MSB`, bit 7) of each byte corresponds to the **leftmost pixel** on the screen.
* **Scanline Length**: Exactly 64 bytes (512 bits) per horizontal scanline.
* **Total Framebuffer Size**: `64 bytes/row * 342 rows = 21,888 bytes` (approx. 21.4 KB).
* **Refresh Rate**: ~60.15 Hz (vertical refresh interval = 16.6 milliseconds).
* **Pixel Clock**: 15.6672 MHz.
* **Horizontal Scan Frequency**: 22.25 kHz (44.93 microseconds per scanline).

### Dual-Access RAM Interleaving
Memory access on the Macintosh motherboard is interleaved between the CPU and the video hardware:
1. Every 4 CPU clock cycles (at 7.8336 MHz), the hardware PAL/BBU takes control of the memory bus to read a 16-bit word from the active screen buffer into a high-speed shift register.
2. The remaining memory cycles belong to the 68000 CPU.
3. Because the hardware reads directly from RAM, any CPU write to the active video memory address (`ScrnBase`) takes immediate visual effect on the CRT phosphor.

### Base Address Mapping (`ScrnBase` at `$0824`)
The screen buffer is located at the top of physical RAM (just below the sound buffer and diagnostic scratchpad):
* **4 MB Macintosh**: `ScrnBase = $003FA700`
* **2.5 MB Macintosh**: `ScrnBase = $0027A700`
* **2 MB Macintosh**: `ScrnBase = $001FA700`
* **1 MB Macintosh**: `ScrnBase = $000FA700`

The kernel obtains this address at boot time via the low-memory global:
```c
uint8_t *front = (uint8_t *)LM_LONG(0x824); /* ScrnBase */
```

---

## 2. PlusOS Tear-Free Double-Buffered Video Pipeline

### The Problem: CRT Beam Tearing
When an application draws directly into `ScrnBase` (e.g. erasing a window, rendering lines, moving a cursor), the CRT scanning beam catches the CPU mid-draw. The user sees an annoying flicker as the old frame is partially erased before the new frame is fully rasterized.

### The Solution: Synchronized Double Buffering (`kernel/video.c`, `kernel/vblit.S`)
PlusOS solves display flicker by allocating a dedicated **off-screen back buffer** in RAM and copying complete frames during the Vertical Blanking Interval:

```
[Application / PlusGFX]
        |
        | 1. Render primitives (lines, text, polygons, 3D meshes)
        v
+-------------------------------+
|     Off-Screen Back Buffer    |  (21,888 bytes in RAM)
|     (video_back() / kmalloc)  |
+-------------------------------+
        |
        | 2. video_present() triggered
        v
+-------------------------------+
| Wait for Vertical Blank (VBL) |  (Polls Ticks at $016A until next 60Hz tick)
+-------------------------------+
        |
        | 3. Lock CrsrBusy ($08CD) to prevent mouse XOR mid-copy
        | 4. Execute unrolled 68000 blitter: video_copy() in vblit.S
        v
+-------------------------------+
|   Live Frame Buffer (VRAM)    |  (ScrnBase at $0824)
|   Outruns CRT electron beam!  |
+-------------------------------+
        |
        v
+-------------------------------+
| 512x342 CRT Display (No Tear) |
+-------------------------------+
```

### The Unrolled 68000 Blit Engine (`kernel/vblit.S`)
Copying 21,888 bytes within the vertical blanking interval (~1.2 ms) requires raw speed. PlusOS uses a hand-optimized assembly loop unrolled to **16 longwords (64 bytes = 1 full scanline) per iteration**:

```assembly
        .text
        .globl  video_copy
video_copy:
        movea.l 4(%sp), %a1                 /* %a1 = Destination (front / ScrnBase) */
        movea.l 8(%sp), %a0                 /* %a0 = Source (back buffer) */
        move.l  12(%sp), %d0                /* %d0 = Total bytes (21,888) */
        lsr.l   #6, %d0                     /* %d0 = Number of 64-byte scanlines (342) */
        bra.s   2f
1:      move.l  (%a0)+, (%a1)+              /* 16 longwords = 64 bytes */
        move.l  (%a0)+, (%a1)+
        move.l  (%a0)+, (%a1)+
        move.l  (%a0)+, (%a1)+
        move.l  (%a0)+, (%a1)+
        move.l  (%a0)+, (%a1)+
        move.l  (%a0)+, (%a1)+
        move.l  (%a0)+, (%a1)+
        move.l  (%a0)+, (%a1)+
        move.l  (%a0)+, (%a1)+
        move.l  (%a0)+, (%a1)+
        move.l  (%a0)+, (%a1)+
        move.l  (%a0)+, (%a1)+
        move.l  (%a0)+, (%a1)+
        move.l  (%a0)+, (%a1)+
        move.l  (%a0)+, (%a1)+
2:      dbra    %d0, 1b                     /* Decrement and branch */
        rts
```

### Partial Updates: `video_present_rect(top, bottom)`
For small updates (such as an animated cursor, a blinking prompt, or a digital clock), copying all 21,888 bytes is wasteful. PlusOS provides bounded row presentations:
```c
void video_present_rect(int top, int bottom)
{
    if (!back) return;
    if (top < 0) top = 0;
    if (bottom > FB_ROWS) bottom = FB_ROWS;
    if (top >= bottom) return;
    blit(front + top * FB_ROWBYTES, back + top * FB_ROWBYTES,
         (uint32_t)(bottom - top) * FB_ROWBYTES);
}
```

---

## 3. PlusGFX 2D Rendering Engine (`<plusos/gfx.h>`)

PlusOS provides `PlusGFX` (`libplusgfx.a`), a hardware-accelerated 2D graphics engine providing arbitrary surface creation, clipping, geometric primitives, 16-shade dithering, and polygon rasterization:

### Surface Representation (`GSurface`)
```c
typedef struct GSurface {
    uint8_t *bits;          /* 1 bpp bitmap data (1 = black) */
    int16_t  rowbytes;      /* Bytes per row (even, e.g. 64) */
    int16_t  w, h;          /* Dimensions in pixels */
    int16_t  owned;         /* 1 if allocated by gs_init */
} GSurface;
```

### 16-Shade 4×4 Ordered Bayer Dithering
To simulate grayscale photography, shaded polygons, and depth on a strictly 1-bit monochrome display, PlusOS implements a 4×4 Bayer matrix yielding **17 discrete shades** (0 = pure white, 16 = solid black):

```
Bayer Matrix Thresholds (4x4):
   0   8   2  10
  12   4  14   6
   3  11   1   9
  15   7  13   5
```

```c
/* Fill a rectangle with a dithered shade (0 = white .. 16 = black) */
void gfx_dither_rect(int l, int t, int r, int b, int shade);

/* Fill an arbitrary N-sided polygon with a dithered shade */
void gfx_poly_dither(const int *pts, int npts, int shade);
```

### Arbitrary Polygon & Hexagon Rasterizer
PlusOS includes a high-performance scanline polygon renderer capable of filling arbitrary concave/convex polygons and regular N-gons:
```c
/* Draw an outlined polygon */
void gfx_poly_frame(const int *pts, int npts, int black);

/* Solid fill an arbitrary polygon with scanline parity fill */
void gfx_poly_fill(const int *pts, int npts, int black);

/* Generate vertex coordinates for regular N-gons (triangles, pentagons, hexagons) */
void gfx_ngon_pts(int cx, int cy, int radius, int sides, int angle_deg, int *pts_out);

/* Draw and fill a regular hexagon with custom border and Bayer shade */
void gfx_hexagon(int cx, int cy, int radius, int angle_deg, int shade, int border);
```

---

## 4. Plus3D Fixed-Point 3D Graphics Engine (`<plus3d/plus3d.h>`)

Because the Motorola 68000 processor does not have floating-point hardware (no FPU), floating-point math would result in abysmal frame rates. PlusOS includes **Plus3D**, a real-time 3D projection engine built entirely on **16.16 fixed-point arithmetic**:

```
Fixed-point representation (Fixed32):
Bit 31: Sign bit
Bits 16-30: 15-bit integer part
Bits 0-15:  16-bit fractional part (1.0 = 65536 = 0x00010000)
```

### The 3D Rendering Pipeline (`cube3d.c`)
```
3D Model Vertices (X, Y, Z)
            |
            v
[Rotation Matrix: Pitch, Yaw, Roll via 256-entry Fixed-Point Sine Table]
            |
            v
[Camera Transformation: Translation (X+dx, Y+dy, Z+dist)]
            |
            v
[Perspective Division: ScreenX = (X * FocalLength) / Z + CenterX]
                       ScreenY = (Y * FocalLength) / Z + CenterY
            |
            v
[Backface Culling: Cross Product of Edge Vectors (Z-Normal > 0)]
            |
            v
[PlusGFX Rasterizer: Wireframe Edges (gfx_line) or Shaded Polys (gfx_poly_dither)]
            |
            v
[Double-Buffer Page Flip: gfx_present()]
```

### Built-in 3D Geometric Meshes
Plus3D includes procedural generators for classic 3D primitives:
* `mesh_build_cube`: 8 vertices, 6 quad faces.
* `mesh_build_hex_prism`: Hexagonal prism (12 vertices, 8 faces).
* `mesh_build_pyramid`: 4-sided pyramid (5 vertices, 5 faces).
* `mesh_build_octahedron`: 8-sided diamond (6 vertices, 8 faces).
* `mesh_build_torus`: Multi-segment 3D donut ring.

---

## 5. Modular & Color Macintosh Framebuffer Reference


While the compact Macintosh Plus and SE use the 1-bit monochrome video architecture documented above, modular Macintosh computers (Mac II, IIci, Quadra 700/800/900) introduced separate hardware framebuffers:

### Direct Access Frame Buffers (DAFB & Civic)
* **Video Hardware**: Apple DAFB (Direct Access Frame Buffer) and Civic video chips.
* **Color Depths**: 1 bpp (monochrome), 2 bpp (4 colors), 4 bpp (16 colors), 8 bpp (256 indexed colors from CLUT), and 24 bpp direct RGB color.
* **RAMDAC / CLUT (Color Look-Up Table)**:
  * Uses Brooktree Bt473, Sierra, or AMD RAMDACs.
  * Colors are programmed by writing red, green, and blue intensity bytes to the RAMDAC index registers (`$50F14000`).
* **Monitor Sense Lines (AltSense)**:
  * The video hardware reads 3 monitor sense pins on the DB-15 video connector at boot.
  * Sense configurations identify whether an Apple 12" Monochrome (640×480), 13" High-Res RGB (640×480), Portrait Display (640×870), or Two-Page Display (1152×870) is attached.
  * Dot clock generators program video PLL frequencies accordingly (e.g. 30.24 MHz for 13" RGB).

For PlusOS, the modular driver architecture provides a template for expanding PlusOS in future revisions to run on modular Mac II / Quadra systems with full color support.

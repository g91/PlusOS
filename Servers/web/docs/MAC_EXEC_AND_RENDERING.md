# PlusOS Architecture: Classic Mac `'APPL'` Execution, Video Rendering, 2D/3D Polygons & Hexagons, and System 7.1 Features

This document provides the complete technical reference for how **PlusOS** launches executables (both native `.pex` binaries and Classic Macintosh `'APPL'` / `'CODE'` applications), how full-screen tear-free rendering works on the Macintosh Plus and Macintosh SE, how 2D and 3D squares, polygons, and hexagons are constructed and rasterized with 1-bit ordered Bayer dithering, and how System 7.1 (`F:\sys71src`) and Macintosh ROM (`F:\mac_rom`) features and emulators are integrated.

---

## 1. How Applications Are Launched in PlusOS

PlusOS supports two executable formats directly from the shell, the Desktop GUI, or `plusos->run()`:

1. **Native PlusOS Executables (`.pex` / `PLOS`)** — Position-independent, self-relocating 68000 user-mode (`SR = $0000`) binaries.
2. **Classic Macintosh Applications (`'APPL'` / `'CODE'` resources)** — Standard Macintosh System 6 / System 7 68000 applications stored in an HFS Resource Fork, raw `.rsrc` file, or MacBinary (`.bin`) container.

---

### 1.1 Native PlusOS Executable Format (`.pex`)

Every `.pex` binary starts with the 8-byte header emitted by `crt0.S`:

| Offset | Size | Content | Description |
|---|---|---|---|
| `0x00` | 4 bytes | `BRA.W _start` (`$6000 xxxx`) | Direct branch to entry point |
| `0x04` | 4 bytes | `"PLOS"` (`$504C4F53`) | PlusOS executable magic signature |

#### Launch Sequence (`kernel/exec.c` -> `proc_spawn` -> `crt0.S`)
1. **Resolution (`load_app` in `kernel/exec.c`)**:
   - Searches HFS `:bin:<name>`, root `:<name>`, or explicit HFS paths (`:Games:CUBE3D`, `:Desktop:...`).
   - Falls back to the raw 4-block (`PLDR`) directory at `APPS_BLOCK` (holds up to 85 raw apps).
2. **Process Slot & Stack Setup (`kernel/proc.c`)**:
   - Allocates a process control block (`struct proc`) and a 24 KB user stack (`PROC_STACK_SIZE`) plus a 4 KB kernel supervisor stack (`KSTACK_SIZE`) for preemptive multitasking.
   - Places the `PlusOSAPI *` service table pointer (`0x00000F00`), `argc`, and `argv` on the user stack.
3. **Self-Relocation (`crt0.S`)**:
   - Linked at base address `0` (`app.ld`) with `--emit-relocs`, compressed by `tools/mkreloc.py` into a table of 32-bit offsets appended at `__reloc_start` (`__bss_start` before clearing BSS).
   - At startup, `_start` computes the runtime load delta `D0 = PC - 2`, walks the relocation table adding `D0` to every 32-bit pointer in `.text`/`.data`, zeroes `.bss`, saves `PlusOSAPI *` to `plusos`, and calls `main(argc, argv)`.
4. **Privilege & ROM Trap Gate (`kernel/aline.S`)**:
   - User processes run in **68000 User Mode** (`SR = $0000`, using `USP`).
   - System calls use `TRAP #1` (`vector 33` -> `trap1_entry` -> `sys_dispatch`).
   - Calling any Macintosh ROM A-line trap (`$Axxx`) from user mode enters `aline_entry` (vector 10 at `$0028`). If the caller was in user mode, `aline_entry` switches to supervisor mode, copies 36 bytes of trap arguments from `USP` to the supervisor stack, switches `SP` to the shared user stack so QuickDraw and File Manager stack references remain valid, executes the real ROM trap (`$Axxx`), and returns to user mode via `RTE`.

---

### 1.2 Classic Macintosh `'APPL'` Executable Loader (`kernel/macloader.c`)

Based on Apple's **Segment Loader** (`F:\mac_rom\plus\os\segment_loader.s` and `F:\sys71src\Internal\Asm\SegLoadPriv.a`), Classic Macintosh applications do not store code in the HFS Data Fork. Instead, their executable code lives in `'CODE'` resources inside the **Resource Fork**.

```
+-------------------------------------------------------------------+
|                   Classic Macintosh 'APPL' File                   |
|                                                                   |
|  Data Fork:     Usually empty (0 bytes)                           |
|  Resource Fork:                                                   |
|    +-- Resource Header (256 bytes: dataOff, mapOff, dataLen, ...) |
|    +-- Resource Data   (4-byte length prefix + payload per rsrc)  |
|    |     +-- 'CODE' ID=0 (A5 World layout + Jump Table)           |
|    |     +-- 'CODE' ID=1 (Main entry segment)                     |
|    |     +-- 'CODE' ID=2..N (Additional demand-loaded segments)   |
|    +-- Resource Map    (Type List -> Reference List -> Name List) |
+-------------------------------------------------------------------+
```

#### Step 1: Resource Fork & MacBinary Detection (`mac_is_appl_file`)
`macloader.c` inspects three formats automatically:
1. **HFS Resource Fork (`OpenRF`)**: Reads the 16-byte Resource Fork header via the ROM File Manager's `OpenRF` call and checks Finder info (`GetFInfo` -> `fdType == 'APPL'`) or the presence of `'CODE'` resources.
2. **Raw Resource Fork in Data Fork (`.rsrc`)**: Validates the big-endian Resource Header (`dataOff >= 0x100`, `mapOff > dataOff`, `mapOff + mapLen <= fileSize`) and searches the Resource Map Type List for `'CODE'` (`0x434F4445`).
3. **MacBinary I/II/III Container (`.bin`)**: Checks byte 0 (`0`), byte 74 (`0`), Finder Type at bytes `65..68` (`'APPL'`), Data Fork length at `83..86`, and locates the Resource Fork at `128 + ((dataLen + 127) & ~127)`.

#### Step 2: Parsing `'CODE'` Resource 0
`'CODE'` 0 is always 16 bytes of header followed by the initial **Jump Table**:

```c
struct mac_code0_hdr {
    uint32_t aboveA5;   /* Bytes required at and above A5 (QD ptr + Jump Table) */
    uint32_t belowA5;   /* Bytes required below A5 (application global variables) */
    uint32_t jtSize;    /* Size in bytes of the Jump Table in CODE 0 */
    uint32_t jtOffset;  /* Offset from A5 to start of Jump Table (usually 32 = $20) */
};
```

#### Step 3: Constructing the Macintosh A5 World
The 68000 `A5` register is the anchor for all Macintosh application globals, QuickDraw globals, and inter-segment procedure calls (`-ffixed-a5` in PlusOS preserves `A5` across user C code):

```
        Low MemoryAddresses
        +---------------------------------------+  <-- A5 - belowA5
        |       Application Global Variables    |
        |       (and QuickDraw Globals:         |
        |        randSeed, screenBits, arrow,   |
        |        dkGray, ltGray, gray, black,   |
        |        white, thePort at 0(A5))       |
A5 ---> +---------------------------------------+  <-- CurrentA5 ($0904)
        | 0(A5): Pointer to QuickDraw thePort   |
        | 4(A5)..31(A5): App Parameters / Finder|
        +---------------------------------------+  <-- A5 + CurJTOffset ($0934) [usually A5 + $20]
        |              Jump Table               |
        |   Entry 0: Main entry point (CODE 1)  |
        |   Entry 1..K: Inter-segment stubs     |
        +---------------------------------------+  <-- A5 + aboveA5
        High Memory Addresses
```

`macloader.c` allocates a contiguous block of `belowA5 + aboveA5` bytes, zeroes the globals below `A5`, sets `A5 = base + belowA5`, copies the Jump Table from `'CODE'` 0 into `A5 + jtOffset`, and updates Macintosh low-memory globals:
- `CurrentA5` (`$0904`) = `A5`
- `CurJTOffset` (`$0934`) = `jtOffset` (typically `0x0020`)
- `CurApName` (`$0910`) = Pascal string of the launched application name
- `CurMap` (`$0A5A`) = Resource fork reference

#### Step 4: Loading `'CODE'` Segments & Jump Table Patching
Each standard `'CODE'` segment (`ID >= 1`) has a 4-byte header:
- `uint16_t jtByteOffset`: Offset into the Jump Table for this segment's first routine.
- `uint16_t jtEntryCount`: Number of Jump Table entries belonging to this segment.
- Followed immediately by position-independent 68000 machine code at `seg + 4`.

In `'CODE'` 0, each unloaded 8-byte Jump Table entry looks like:
```
Offset 0..1:  routine_offset   (uint16_t byte offset of routine within segment code)
Offset 2..3:  $3F3C            (MOVE.W #segID, -(SP))
Offset 4..5:  segID            (uint16_t segment resource ID, e.g., $0001)
Offset 6..7:  $A9F0            (_LoadSeg A-line trap)
```

When `mac_patch_seg_jt()` loads segment `segID` into memory at `code_base = seg + 4`, it patches every Jump Table entry referencing `segID` into a direct 6-byte absolute jump:
```
Offset 0..1:  segID            (saved segment ID)
Offset 2..3:  $4EF9            (JMP.L abs.L opcode)
Offset 4..7:  code_base + routine_offset (32-bit absolute target address)
```
The entry point of the Macintosh application is `A5 + jtOffset + 2` (the `JMP.L` instruction of the very first Jump Table entry).

#### Step 5: Segment Loader Trap Hooks (`$A9F0`, `$A9F1`, `$A9F4`, `$A9F5`)
Before jumping to the application entry point, `macloader.c` installs native 68000 trap handlers into the Toolbox dispatch table (`$0C00` on Mac Plus, `$0E00` on Mac SE) for:
- **`_LoadSeg` (`$A9F0`)**: Pops the return address and `segID` from the stack, loads `'CODE'` `segID` if not already loaded, patches its Jump Table entries to `JMP.L`, and re-executes the now-patched `JMP.L` at `ret_addr - 6`.
- **`_UnLoadSeg` (`$A9F1`)**: Accepts the routine pointer on the stack and returns cleanly (segments remain pinned in PlusOS RAM).
- **`_GetAppParms` (`$A9F5`)**: Fills the caller's `apName` (`CurApName`), `apRefNum` (`CurMap`), and `apParam` (`0`) pointers.
- **`_ExitToShell` (`$A9F4`)**: Restores the kernel stack pointer `SP`, `A5`, and Toolbox trap vectors, frees all loaded `'CODE'` segments and the A5 World, and returns cleanly to the PlusOS shell or Desktop.

#### Step 6: Using `macrun`
The `macrun` utility (`apps/macrun/macrun.c`) lets you inspect, generate, and run Classic Macintosh `'APPL'` executables:
```sh
macrun --makedemo           # Creates a real Classic Mac 'APPL' (:MacDemo) with CODE 0 + CODE 1
macrun --info :MacDemo      # Inspects Resource Fork, CODE 0 A5 World metrics, and Jump Table
macrun :MacDemo             # Launches the Classic Mac 'APPL' executable
```
You can also type any `'APPL'` filename directly at the PlusOS shell prompt — `kernel/exec.c` automatically invokes `mac_exec_file()` when a file is a Classic Mac `'APPL'`!

---

## 2. Full-Screen Video Rendering & Anti-Flicker Architecture

### 2.1 Compact Macintosh Framebuffer Layout
On both the Macintosh Plus and Macintosh SE, the video hardware scans a linear 1-bit monochrome framebuffer in main RAM:
- **Resolution**: `512 x 342` pixels (`175,104` pixels total).
- **Row Stride (`ROWBYTES`)**: `64` bytes per scanline (`512 / 8`).
- **Total Size (`FB_BYTES`)**: `64 * 342 = 21,888` bytes (`21.375 KB`).
- **Base Address**: Stored in the low-memory global `ScrnBase` (`*(uint8_t **)0x0824`, typically `$7A700` on a 512 KB Mac or `$3FA700` on a 4 MB Mac).
- **Bit Polarity (Paper-White)**:
  - `0` = **White** (paper background)
  - `1` = **Black** (ink foreground)
  - Most significant bit (`0x80`) of byte `0` is pixel `(0, 0)` at the top-left corner.

### 2.2 Double-Buffered Page Flipping (`gfx_backbuffer` -> `gfx_present`)
Drawing directly to `ScrnBase` (`gfx_screen()`) causes visible tearing and flicker because the CRT beam scans across the buffer while an application erases and redraws frames. PlusOS solves this at two levels:

1. **Kernel Video Driver (`kernel/video.c` & `kernel/vblit.S`)**:
   - Maintains a 21,888-byte off-screen `backbuf` aligned to 4 bytes.
   - `video_present()` (`SYS_vpresent` when `r == 0`):
     1. Waits for the **Vertical Blanking Interrupt (VBL)** edge by spinning until `Ticks` (`*(volatile uint32_t *)0x016A`) increments (`wait_vbl()`), unless called within the same 60 Hz tick.
     2. Locks the ROM cursor interrupt via `CrsrBusy` (`LM_BYTE(0x08CD) = 1`) so the VBL cursor task cannot clobber the screen mid-blit.
     3. Calls `video_copy(ScrnBase, backbuf, 21888)` in `kernel/vblit.S`, which is **unrolled to copy 64 bytes (16 longwords = 1 full 512-pixel scanline) per loop iteration** using `move.l (a0)+, (a1)+` and `dbra`, completing the entire 21,888-byte page flip in ~1.2 ms (well inside the VBL interval before the active raster reaches the playfield).
     4. Unlocks `CrsrBusy = 0`.
2. **PlusGFX Userland Framework (`<plusos/gfx.h>`, `src/gfx/gfx.c`)**:
   - Applications call `gfx_open()`, target `gfx_backbuffer()`, draw the entire frame into the off-screen buffer (`back_s`), and call `gfx_present()`.
   - `gfx_present()` copies `back_s` into the kernel backbuffer and invokes `SYS_vpresent` for a single VBL-synced flip.
   - **Automatic Fallback Safeguard**: If legacy code targets `gfx_screen()` directly and calls `gfx_present()`, `gfx_present()` detects `tgt == &screen_s` and synchronizes from `screen_s` instead of overwriting the screen with an empty `back_s`.

---

## 3. Drawing 2D/3D Squares, Polygons, Hexagons & Bayer Dithering

### 3.1 How System 7.1 QuickDraw Draws Polygons (`F:\sys71src`)
In Apple's System 7.1 QuickDraw (`F:\sys71src\Internal\Asm\Polygons.s` and `BitMaptoRegion.a`):
- `OpenPoly` records `LineTo` calls into a variable-length `Polygon` handle (`polySize`, `polyBBox`, `polyPoints[]`).
- `DrawPoly` converts the polygon vertices into sorted edge vectors, maintains an **Active Edge Table (AET)** sorted by horizontal intercept `X` for each scanline `Y`, and fills spans between alternating pairs of edges (even-odd winding rule) clipped against the current `GrafPort`'s `visRgn` and `clipRgn`.

### 3.2 PlusGFX 2D Polygon, Regular N-Gon, & Hexagon Primitives (`<plusos/gfx.h>`)
PlusOS implements fast, zero-allocation scanline polygon and regular N-gon rasterizers directly in `libplusgfx.a`:

| Function | Description |
|---|---|
| `gfx_poly_frame(pts, n, col)` | Draws the closed wireframe outline of an `n`-vertex 2D polygon (`pts = [x0,y0, x1,y1, ...]`). |
| `gfx_poly_fill(pts, n, col)` | Scanline even-odd polygon fill (`0` = white, `1` = black, `2` = 50% checkerboard gray). |
| `gfx_poly_dither(pts, n, shade)` | Scanline polygon fill shaded with **16-level (`0..15`) 4x4 ordered Bayer dithering**. |
| `gfx_ngon_pts(cx, cy, r, sides, angle_deg, out_pts)` | Computes the vertices of any regular polygon (`3` = triangle, `4` = rotated square/diamond, `5` = pentagon, `6` = hexagon, `8` = octagon) using a 1024-scale integer trig lookup table (no FPU required). |
| `gfx_ngon_frame(cx, cy, r, sides, angle_deg, col)` | Draws a regular `sides`-gon outline rotated by `angle_deg` degrees. |
| `gfx_ngon_fill(cx, cy, r, sides, angle_deg, shade)` | Fills a regular `sides`-gon with Bayer dither shade `0..15`. |
| `gfx_hexagon(cx, cy, r, flat_top, fill_shade, border_col)` | Dedicated helper to draw flat-top (`flat_top=1`) or pointy-top (`flat_top=0`) 2D hexagons (e.g., for hex-grid strategy games or honeycombs). |
| `gfx_dither_rect(l, t, r, b, shade)` | Fills a 2D rectangle/square with 16-shade (`0..15`) ordered Bayer dithering. |
| `gfx_circle` / `gfx_fill_circle` | Midpoint Bresenham circle outline and solid/gray fill. |
| `gfx_round_rect` / `gfx_fill_round_rect` | Classic Mac-style rounded rectangle outline and fill. |

### 3.3 Plus3D Engine (`<plus3d/plus3d.h>`) — 3D Squares, Cubes, Hexagonal Prisms & Polygons
PlusOS bundles the **Plus3D** 3D math, camera, lighting, and mesh engine directly into `libplusgfx.a`:

1. **16-Shade 4x4 Ordered Bayer Dithering**:
   Because the Mac Plus/SE screen is strictly 1-bit (`black` or `white`), Plus3D simulates **16 distinct grayscale lighting levels (`shade 0..15`)** using a 4x4 ordered Bayer threshold matrix:
   ```
   Bayer 4x4 Matrix:
   [  0,  8,  2, 10 ]
   [ 12,  4, 14,  6 ]
   [  3, 11,  1,  9 ]
   [ 15,  7, 13,  5 ]
   ```
   For each pixel `(x, y)` inside a 3D polygon face, if `shade > Bayer[y & 3][x & 3]`, the pixel is drawn black (`1`); otherwise white (`0`).

2. **3D Pipeline (`render_mesh` in `src/gfx/plus3d.c`)**:
   - **Model-View Transform**: Transforms each 3D vertex `Vec3` by the object's rotation/translation `Mat4` and the `Camera`'s view matrix.
   - **Backface Culling**: Computes the 3D face normal via cross product `(v1 - v0) x (v2 - v0)` and culls faces pointing away from the camera (`dot(normal, view_vec) >= 0`).
   - **Lambertian Directional Lighting**: Computes diffuse intensity `dot(normal, light_dir)` + ambient term, mapping each polygon face to a Bayer dither shade `1..14`.
   - **Painter's Algorithm Depth Sort**: Sorts visible faces back-to-front by average view-space $Z$ depth before rasterizing each triangle (`fill_tri_dither`) and optional wireframe edge.

3. **Built-In 3D Procedural Meshes**:
   - `mesh_build_cube(s)` — 3D Cube (6 3D square faces, 12 triangles).
   - `mesh_build_box(w, h, d)` — 3D Rectangular Box.
   - `mesh_build_hex_prism(r, h)` — **3D Hexagonal Prism / 3D Hexagon** (top & bottom 6-sided hexagonal caps + 6 rectangular side faces = 24 triangles).
   - `mesh_build_pyramid(base, h)` — 4-sided 3D Pyramid.
   - `mesh_build_octahedron(r)` — 8-sided Regular 3D Octahedron (diamond).
   - `mesh_build_cylinder(r, h, segs)` — `N`-sided 3D Prism / Cylinder (`segs=6` is a 3D hexagonal column).
   - `mesh_build_torus(R, r, r_segs, t_segs)` — 3D Torus ring.
   - `mesh_build_sphere(r, rings, slices)` — 3D UV Sphere.

Run `cube3d` from the shell or Desktop to interactively inspect all 6 3D/2D modes (`1`=3D Cube, `2`=3D Hexagonal Prism, `3`=3D Pyramid, `4`=3D Octahedron, `5`=3D Torus, `6`=2D/3D Polygons & Honeycomb Hexagons, `W`=Wireframe/Solid/Shaded, `A`=Auto-spin).

---

## 4. Console History, Tab Completion, System 7.1 Features & Emulators

### 4.1 Persistent 20-Command Console History (`kernel/kbd.c` & `kernel/winsrv.c`)
- Pressing **Up Arrow** (`0x1E`) and **Down Arrow** (`0x1F`) in either the full-screen console (`kbd_line`) or a Desktop terminal window (`term_readline`) cycles through the last **20 commands**.
- Every executed command is immediately persisted to the boot HFS hard drive in `":PlusOS History"`.
- On boot (`main.c` -> `kbd_history_load()`), PlusOS reloads `":PlusOS History"`, so your command history survives reboots and power cycles.
- Run `history` in the shell to list all 20 saved commands, or `history -c` to clear them.

### 4.2 Tab Auto-Completion (`kernel/kbd.c` & `kernel/winsrv.c`)
- Pressing **Tab** (`0x09`) while typing in the console or a Desktop terminal window auto-completes:
  - **First word (Command)**: Matches shell built-ins (`help`, `history`, `ls`, `cat`, `clear`, `exit`, `jobs`, `fg`, `reboot`, `off`), executables in `:bin:`, root files, and the raw app store.
  - **Subsequent words or `:path:` arguments**: Matches files and folders in the target HFS directory (`:`, `:bin:`, `:Desktop:`, `:Games:`).
- **Behavior**:
  - Single match: Completes the word (and appends a trailing space for commands or `:` for directories).
  - Multiple matches: Extends the input to the longest common prefix, displays all matching candidates on the bottom status bar, and cycles through each candidate on repeated **Tab** presses.

### 4.3 System 7.1 (`F:\sys71src`) Features Added to PlusOS
1. **Hardware PWM Sound Driver & 4-Voice Synthesizer (`kernel/sound.c`, `SYS_sound`, `apps/sound`)**:
   - Drives the Compact Mac 370-byte main sound buffer at `SoundBase` (`$0266`, 22.255 kHz sample rate, 1 byte per horizontal scanline) and VIA vBufA volume bits (`$EFE1FE`).
   - Supports square, triangle, sawtooth, and noise waveforms, non-blocking VBL-driven melodies, and an interactive keyboard piano (`sound` app).
2. **VBL Stack Sniffer (`kernel/tick.c`)**:
   - Like System 7.1 `VBLTask.a`, checks the 68000 stack pointer at 60 Hz against `StkLowPt` (`$0110`) and user stack guard words to detect stack corruption early.
3. **System-Wide Scrap / Clipboard Manager (`kernel/scrap.c`, `SYS_scrap`, `apps/clip`)**:
   - Implements the Macintosh Scrap Manager (`ZeroScrap`, `PutScrap`, `GetScrap`, `UnloadScrap`) backed by RAM and persisted to `":Clipboard File"` on the boot HFS disk.
4. **Classic Macintosh Desk Accessories (`calc`, `notes`, `puzzle`)**:
   - Accessible from the **PlusOS** menu on the Desktop or from the shell:
     - `calc` — Interactive 4-function Calculator Desk Accessory window + CLI expression evaluator.
     - `notes` — 8-page persistent Note Pad Desk Accessory saved to `":Note Pad File"`.
     - `puzzle` — Classic 4x4 (15-tile) sliding Puzzle Desk Accessory.

### 4.4 Built-In Emulators (`chip8`, `emu6502`, `basic`)
1. **`chip8` (`apps/chip8/chip8.c`)**:
   - Complete **CHIP-8 Virtual Machine & Arcade Emulator** (`35` opcodes, `4 KB` RAM, `16` registers, `60 Hz` delay/sound timers with PlusOS PWM audio, `64x32` display scaled `6x` to `384x192` full-screen tear-free) with **5 built-in playable ROMs** (*Pong*, *Breakout*, *Maze*, *Space Invaders*, *Sierpinski*) plus external `.ch8` file loading.
2. **`emu6502` (`apps/emu6502/emu6502.c`)**:
   - **MOS 6502 CPU & Apple I Woz Monitor Emulator** (`64 KB` RAM, Apple I PIA `$D010`/`$D012` terminal I/O, PlusGFX graphics MMIO at `$C000`, single-step CPU debugger) with **3 built-in 6502 machine-code programs** (*Sieve of Eratosthenes Benchmark*, *6502 Graphics Starburst*, *Fibonacci*) plus Apple I Woz Monitor hex entry.
3. **`basic` (`apps/basic/basic.c`)**:
   - **PlusOS Retro BASIC Interpreter** supporting line numbers, `PRINT`, `LET`, `IF/THEN`, `FOR/NEXT`, `GOTO`, `GOSUB/RETURN`, `INPUT`, `RND`, and full-screen PlusGFX graphics commands (`CLS`, `LINE`, `RECT`, `CIRCLE`, `POLY`, `HEXAGON`, `WAITKEY`) with **3 built-in demos** (`DEMO 1` Honeycomb Hexagons, `DEMO 2` 3D Wireframe Cube, `DEMO 3` Starfield).

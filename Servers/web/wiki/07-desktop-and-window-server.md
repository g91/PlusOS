# Desktop GUI & Multi-OS Theme Engine

The PlusOS Desktop (`apps/system/desktop`) is a lightweight, responsive windowing environment built directly upon the kernel's window server (`SYS_win`).

---

## 1. Tear-Free Double-Buffered Compositor

On an 8 MHz 68000 without dedicated graphics acceleration, screen tearing during window drags is normally severe. PlusOS eliminates tearing completely:
- **Off-screen Backing Stores**: Each window and desk accessory renders into an off-screen surface (`GSurface`).
- **Scanline Dirty Rect Tracking**: Only modified rectangular areas are marked dirty.
- **Unrolled Blitter (`vblit.S`)**: A hand-coded 68000 assembly blitter pushes 64-byte scanlines using `MOVEM.L` instructions synchronized to the hardware Vertical Blank (VBL) beam.
- Result: Silky-smooth 60 FPS window dragging and flicker-free menu pull-downs.

---

## 2. Multi-OS Theme Switching

PlusOS features a real-time skinning engine that transforms the look and feel of the entire interface without rebooting:

| Theme | Key Shortcut | Visual Style |
|---|---|---|
| **Mac OS 7.1** | `Cmd + 1` | Authentic System 7 pinstripe titlebars, Chicago typography, classic close box. |
| **Commodore Amiga** | `Cmd + 2` | Amiga Workbench 2.0 / 3.0 styling, blue/orange/gray palette, iconic gadget borders. |
| **Windows 95** | `Cmd + 3` | Classic Redmond 3D beveled borders, taskbar, start button, `X` close buttons. |
| **Windows 98** | `Cmd + 4` | Gradient titlebars, active window highlighting, retro taskbar clock. |
| **Vista / 7 Aero** | `Cmd + 5` | Glass-styled transluscent-simulated borders, modern titlebar styling. |
| **Hacker CRT** | `Cmd + 6` | Monospaced cyber aesthetic with real-time green/white digital Matrix rain falling in the wallpaper background! |

Themes can be changed on the fly from the **Theme** menu, the **Settings...** window, or the `theme` command-line utility.

---

## 3. Built-in Applications & Desk Accessories

- **Programs**: Categorized launcher displaying all native binaries and games in `:bin:`.
- **Files**: Full visual HFS file manager for browsing folders, reading files, and launching applications.
- **Task Manager (`taskman`)**: Live GUI process monitor with real-time graphical CPU usage bars and process termination.
- **Calculator (`calc`)**: System 7-style desk accessory with arithmetic functions.
- **Note Pad (`notes`)**: Multi-page scratchpad for taking notes, saved directly to disk.
- **15-Puzzle (`puzzle`)**: The beloved classic sliding tile game from the original 1984 Macintosh System Software.
- **Hex Editor (`hexedit`)**: Dual-pane hex and ASCII inspector/editor for examining raw binary files.

---

## Navigation

- **Previous Chapter**: [← 06. Preemptive Multitasking & IPC Subsystems](/wiki/06-multitasking-and-ipc)
- **Next Chapter**: [08. Classic Mac 'APPL' Executable Loader →](/wiki/08-classic-mac-loader)
- **Table of Contents**: [Return to Getting Started](/wiki/01-getting-started)

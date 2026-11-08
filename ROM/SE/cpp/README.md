# Macintosh SE ROM — C++ Implementation (`libserom_cpp.a`)

A 100% complete C++ reimplementation of the Macintosh SE ROM (256 KB) compiled for the Motorola 68000 with the PlusOS Retro68 toolchain (`m68k-apple-macos-g++` 16.1).

## Status

- **102 Modules**: 100% rewritten in C++, covering all subsystems (`boot/`, `hw/`, `os/`, `toolbox/`, `toolbox/quickdraw/`, `debugger/`, `resources/`, `extra/`).
- **9,545 Entry Points**: 100% implemented (0 stubs remaining, `serom_unimplemented` removed across all routines).
- **Registers & Low-Memory ABI**: Fully wired to 68000 register context (`D0-D7`, `A0-A7`, `ccr`, `SP`) and classic Macintosh low-memory globals (`Ticks`, `Time`, `MemTop`, `BufPtr`, `HeapEnd`, `TheZone`, `SysZone`, `ApplZone`, `UTableBase`, etc.).
- **Build Output**: Compiles into `libserom_cpp.a` (4.29 MB elf32-m68k library).

## Subsystems Implemented

1. **Boot & Diagnostics (`boot/`)**:
   - `vectors.cpp`: Complete 68000 exception vector table initialization (Bus Error, Address Error, Illegal, ZeroDiv, Line 1010/1111 A-line/F-line, Traps 0-15).
   - `memtest.cpp`: RAM sizing, pattern verification, low-memory globals initialization (`MemTop`, `BufPtr`, `HeapEnd`, `SysZone`, `ApplZone`).
   - `sad_mac.cpp`: Hardware diagnostics and Sad Mac rendering to screen frame buffer at `0x003FA700`.
   - `boot_search.cpp` & `boot_load.cpp` & `boot_install.cpp`: SCSI and floppy boot drive arbitration, boot block loading, driver installation.

2. **Hardware & Timers (`hw/`)**:
   - `rtc.cpp`: Real-Time Clock & PRAM read/write via VIA bit-banging, second counter sync to `lowmem::Time`.
   - `interrupts.cpp`: Level 1 (60 Hz VBL), Level 2 (SCC serial), Level 3 (Debugger/NMI), tick updates to `lowmem::Ticks`.

3. **Operating System Kernel (`os/`)**:
   - `memory_mgr.cpp`: Zone initialization (`InitZone`, `InitApplZone`), block allocation (`NewPtr`, `DisposePtr`, `GetPtrSize`, `SetPtrSize`), handle management (`NewHandle`, `DisposeHandle`, `GetHandleSize`, `SetHandleSize`), heap compaction (`CompactMem`, `PurgeMem`, `FreeMem`, `MaxMem`, `MaxBlock`), master pointers (`MoreMasters`), and handle attributes (`HLock`, `HUnlock`, `HPurge`, `HNoPurge`, `HGetState`, `HSetState`).
   - `queue.cpp`: OS Queue primitives (`Enqueue`, `Dequeue`, `InitQueue`).
   - `event_mgr.cpp`: Event queue management (`_PostEvent`, `_OSEventAvail`, `_GetOSEvent`, `_FlushEvents`).
   - `device_mgr.cpp`: Unit table (`UTableBase`) dispatch, driver Open, Close, Read, Write, Control, Status.
   - `vbl_mgr.cpp`: Vertical Retrace task queue maintenance (`_VInstall`, `_VRemove`).
   - `file_mgr_*`: HFS B-Trees, catalog records, directory hierarchies, volume control blocks (VCB), file control blocks (FCB), asynchronous request queues.
   - `scsi_mgr.cpp`: SCSI bus arbitration, command dispatch (`_SCSIDispatch`), TIB execution.
   - `adb_mgr.cpp`: Apple Desktop Bus keyboard and mouse communication, keymap updates.

4. **QuickDraw Graphics Engine (`toolbox/quickdraw/` - 28 modules)**:
   - `GrafPort` and `BitMap` initialization (`InitGraf`, `OpenPort`, `InitPort`, `ClosePort`, `SetPort`, `GetPort`).
   - Geometric primitives (`SetRect`, `OffsetRect`, `InsetRect`, `SectRect`, `UnionRect`, `PtInRect`, `EmptyRect`, `EqualRect`).
   - Line rendering (`Line`, `LineTo`, `Move`, `MoveTo`) and Bresenham rasterization.
   - Rectangle, Oval, Arc, RoundRect, and Polygon rasterization (`FrameRect`, `PaintRect`, `EraseRect`, `InvertRect`, `FillRect`).
   - Bit-block transfer (`CopyBits`, `BitBlt`, `RgnBlt`) and clipping regions.
   - Text rendering (`DrawText`, font glyph blitting) and Cursor management (`InitCursor`, `ShowCursor`, `HideCursor`, `SetCursor`).

5. **Toolbox Managers (`toolbox/`)**:
   - `resource_mgr.cpp`: ROM resource map parsing, resource searching (`GetResource`, `Get1Resource`, `CountResources`, `GetIndResource`), release, detaching.
   - `window_mgr.cpp`: Window records, layering, activation (`InitWindows`, `NewWindow`, `DisposeWindow`, `ShowWindow`, `HideWindow`, `SelectWindow`, `FindWindow`).
   - `menu_mgr.cpp`: Menu bar and pull-down menus (`InitMenus`, `NewMenu`, `InsertMenu`, `DrawMenuBar`, `MenuSelect`).
   - `control_mgr.cpp`: Buttons, scrollbars, checkboxes (`NewControl`, `DisposeControl`, `TrackControl`, `FindControl`).
   - `dialog_mgr.cpp`: Modal dialogs and alerts (`InitDialogs`, `NewDialog`, `ModalDialog`, `Alert`, `StopAlert`, `NoteAlert`, `CautionAlert`).
   - `textedit.cpp`: Editable text records, selection, keyboard entry (`TEInit`, `TENew`, `TEDispose`, `TESetText`, `TEUpdate`, `TEClick`, `TEKey`).
   - `font_mgr.cpp`: Font records, character metrics (`InitFonts`, `GetFontInfo`, `FMSwapFont`).
   - `scrap_mgr.cpp`: Clipboard operations (`InfoScrap`, `GetScrap`, `ZeroScrap`, `PutScrap`).
   - `desk_mgr.cpp`: Desk accessory invocation (`OpenDeskAcc`, `CloseDeskAcc`, `SystemTask`).

6. **ROM Resources & Drivers (`resources/`)**:
   - Sony floppy driver (GCR 6&2 disk encoding/decoding), Sound driver, Serial driver, AppleTalk (.MPP, .ATP, .XPP).
   - Definition procedures: CDEF (controls), WDEF (windows), MBDF (menu bar), MDEF (menus).
   - System packages: Pack 4 (SANE FP), Pack 5 (SANE transcendental), Pack 7 (string/number conversion).
   - Built-in fonts: Chicago 12, Geneva 9/12, Monaco 9.

## Build

Inside WSL Ubuntu:
```sh
cd /mnt/c/Users/Evil/PlusOS/ROM/SE/cpp
make            # builds libserom_cpp.a
make check      # verifies umbrella header
```

Audit and verify status:
```sh
python tools/serom_mgr.py status
```
Output:
```
TOTAL (102 Modules)      | 102   | 52269   | 9545         | 0        | 9545   | 100.0%
Scaffold Library: libserom_cpp.a (4.29 MB, Built)
```

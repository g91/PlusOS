# PlusOS Disk Studio, Dynamic Multi-Boot & Mac OS Return Utility

## 1. Overview

**PlusOS Disk Studio** (`tools/disk_studio/PlusOSDiskStudio.exe`) is a high-performance native C++ Win32 application designed to inspect, explore, modify, and manage Classic Macintosh and PlusOS disk images (`.hda`, `.img`, `.dsk`, `.raw`).

Key capabilities:
- **Instant Dual-Pane Explorer**: Visual folder tree hierarchy (`SysTreeView32`) and detailed file list (`SysListView32`) with Mac FourCC file types (`APPL`, `PLOS`, `TEXT`), creators, data fork sizes, resource fork sizes, and modification timestamps.
- **Drag-and-Drop Import**: Drag any file(s) from Windows Explorer directly into the application window to immediately inject them into the active HFS directory on the disk image. Preserves MacBinary II (`.bin`) headers, PlusOS executables (`.pex` -> `PLOS`/`PLOS`), text files (`.txt`/`.cfg` -> `TEXT`/`ttxt`), and raw resource forks.
- **Dynamic Multi-Boot Manager**: Edit `multiboot.cfg` directly through the GUI to add, reorder, and configure boot entries for different operating systems (System 6.0.8, System 7.1, System 7.5.5, A/UX, PlusOS Shell, PlusOS Desktop) across any SCSI ID (0–7).
- **One-Click "Return to PlusOS" Injector**: Injects a genuine Classic Macintosh application (`APPL` / `PLOS`) into any hard drive image or floppy disk image. When double-clicked from Mac OS, it automatically configures XPRAM and restarts the computer directly into PlusOS!

---

## 2. Dynamic Multi-Boot System

PlusOS features a configurable multi-boot system driven by `multiboot.cfg` on the boot volume root.

### `multiboot.cfg` Configuration Format

```ini
# PlusOS Multi-Boot Configuration
boot_pref=2

[entry]
name=System 7.5.5
id=3

[entry]
name=System 7.1
id=2

[entry]
name=System 6.0.8
id=1

[entry]
name=PlusOS Shell
id=shell

[entry]
name=PlusOS Desktop
id=desktop
```

### Target ID Specifications
- `0` through `7`: Boots the operating system installed on that SCSI ID via ROM chainboot (Mac SE / BlueSCSI).
- `shell` or `-1`: Launches the built-in PlusOS console shell (`shell_main`).
- `desktop` or `-2`: Launches the PlusOS GUI Desktop environment (`desktop`).

### Kernel Menu Features (`kernel/services/menu.c`)
- Reads and parses `:multiboot.cfg` on boot.
- Dynamically renders up to 10 entries with custom labels and target SCSI ID indicators.
- Interactive navigation: numbers `1`–`9`, arrow keys Up/Down, Return/Enter, or mouse clicks.
- If `multiboot.cfg` is not present, falls back gracefully to default system entries.

---

## 3. "Return to PlusOS" Classic Macintosh Application

### How It Works Inside Mac OS
When booted into Classic Mac OS (System 6.0.8, System 7.1, System 7.5.5, System 8):
1. **Drive Queue Detection**: Scans the low-memory Drive Queue Header (`DrvQHdr.qHead` at `$030A`) to find the SCSI drive matching SCSI ID 6 (`refNum = -(33 + 6) = 0xFFD9`).
2. **XPRAM Startup Update**: Updates the extended PRAM default startup record at offset `$78` via `_WriteXPRam` (Trap `$A052`), pointing the startup drive to the PlusOS SCSI ID.
3. **Audio Feedback**: Emits an alert chime via `_SysBeep(30)` (Trap `$A9C8`) to signal switching.
4. **Instant Restart**: Triggers a clean hardware reboot via `_Restart` (Trap `$A888`) or `_SysReset`.
5. Upon reboot, the Macintosh ROM reads XPRAM and boots directly into PlusOS!

### Embedded Payload & Standalone Formats
Located in `tools/return_to_plusos/`:
- `Return to PlusOS.bin`: Standard MacBinary II container ready for drag-and-drop or transfer.
- `ReturnToPlusOS.rsrc`: Raw Macintosh resource fork containing `CODE` 0, `CODE` 1, `ICN#` (custom PlusOS desktop icon), `BNDL`, `FREF`, and `vers`.
- `return_to_plusos_payload.h`: Precompiled byte array embedded directly into `PlusOSDiskStudio.exe` for zero-dependency injection into any image.

---

## 4. PlusOS Disk Studio GUI Guide

### Launching the Application
```cmd
tools\disk_studio\PlusOSDiskStudio.exe [image.hda]
```

### Toolbar Actions
- **Open Image**: Browse and mount any `.hda`, `.img`, `.dsk`, or `.raw` image.
- **Inject Return to PlusOS**: Injects `Return to PlusOS` directly into `:Desktop:Return to PlusOS` and the root directory of the currently open disk image.
- **Inject to Floppy / Image...**: Prompts for any external disk or floppy image (`.dsk`, `.img`, `.hda`) and injects the return application into it.
- **Import Files...**: Select one or multiple files to import into the active HFS directory.
- **Export Selected**: Extracts the selected file's data fork to your PC.
- **Delete**: Removes the selected file or directory and frees allocation blocks.
- **Refresh (F5)**: Re-scans the image and refreshes all views.

### Multi-Boot Tab
- **Add OS Entry**: Add new operating system targets.
- **Edit Title/ID**: Cycle target SCSI IDs (0–7, Shell, Desktop) and edit OS display names.
- **Remove Entry**: Remove selected OS from the boot menu.
- **Move Up / Move Down**: Change the display and priority order in the boot menu.
- **Startup Option**: Configure boot preference (`boot_pref`):
  - `Multi-Boot Menu` (default): Always shows boot menu on startup.
  - `PlusOS Shell`: Boots directly to command prompt.
  - `PlusOS Desktop`: Boots directly into graphical desktop.
- **Save to Disk Image**: Flushes the new configuration directly to `multiboot.cfg` on the disk image.

### Command-Line Interface (CLI Mode)
In addition to the GUI, `PlusOSDiskStudio.exe` can be used headlessly in build scripts:
```cmd
PlusOSDiskStudio.exe --inject <image.hda> [scsi_id]
PlusOSDiskStudio.exe --help
```

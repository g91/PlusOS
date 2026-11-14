# PlusOS Disk Studio & Compiler Pipeline

**PlusOS Disk Studio** (`PlusOSDiskStudio.exe`) is the official native Win32/C++ companion suite for Google Antigravity and PlusOS. Located directly in the root of the repository, it gives developers and vintage Macintosh enthusiasts a powerful, multi-threaded GUI to configure, compile, customize, and manage PlusOS disk images (`.hda`, `.img`, `.dsk`).

---

## Architecture Overview

PlusOS Disk Studio separates workflow into two dedicated, focused dialogues:
1. **OS Builder & Dual-Boot System Configurator**: Hardware target selection, multi-boot menu authoring, theme selection, granular software package checklist with size tracking, and an asynchronous multi-threaded compiler with live output streaming.
2. **Hard Drive Explorer & Image Manager**: Dual-pane Macintosh HFS volume browser, drag-and-drop file import/export with automatic FourCC type/creator detection, metadata preview, and Classic Mac OS "Return to PlusOS" injection.

```text
+---------------------------------------------------------------------------------+
|                       PlusOS Disk Studio Architecture                           |
+---------------------------------------------------------------------------------+
|                                                                                 |
|  [OS Builder Dialogue]                     [Hard Drive Explorer Dialogue]       |
|  - Target Architecture (Plus / SE / SE/30) - Dual-Pane HFS Tree & List View     |
|  - Multi-Boot / Dual-Boot Preferences      - Drag-and-Drop Windows Import       |
|  - Theming & Animated Wallpapers           - File Export & Deletion             |
|  - 66-App Package Checklist with Sizes     - Classic Mac OS "Return to PlusOS"  |
|  - Auto-Scanner for build/apps/*.pex         Injector for HDA / Floppy Disks    |
|  - Smart MD5 Compiler Ledger                                                    |
|  - Boot VM (Mini vMac BlueSCSI) Launch                                          |
|  - MSYS2 / WSL Build Backend Choice                                             |
|                                                                                 |
+---------------------------------------+-----------------------------------------+
                                        |
                                        v
                 +---------------------------------------------+
                 | Build Backend (chosen per-build)            |
                 | - MSYS2 Local (tools\winbuild\msys2\)       |
                 | - MSYS2 System (C:\msys64 or C:\msys2)      |
                 | - WSL Ubuntu (legacy / optional)            |
                 | Preference saved: tools\winbuild\build_config.txt
                 +----------------------+----------------------+
                                        |
                                        v
                 +---------------------------------------------+
                 | Async Worker Thread (RunProcessAsync)       |
                 | - Anonymous Pipe (StdOut & StdErr)          |
                 | - Non-blocking Win32 UI Message Pump        |
                 | - Real-time live log streaming              |
                 +----------------------+----------------------+
                                        |
                                        v
                 +---------------------------------------------+
                 | Build Targets:                              |
                 | - Hard Drive: images/PlusOS.hda & BlueSCSI  |
                 | - Floppy: images/PlusOS_Floppy.dsk (800K/1.4M) |
                 +---------------------------------------------+
```

---

## 1. Asynchronous Multi-Threaded Compiler

In previous builds, running compilers or disk generators from a GUI would freeze the main window until completion. PlusOS Disk Studio implements a fully decoupled, multi-threaded execution model:

- **Non-Blocking UI**: Compilation (`make install` or `make floppy`) runs in a dedicated background worker thread (`std::thread`). The main window remains 100% responsive—you can drag, resize, inspect settings, and view logs smoothly without Windows marking the application as "(Not Responding)".
- **Real-Time Log Streaming**: Child process standard output and standard error are captured via Win32 anonymous pipes (`CreatePipe`) and streamed line-by-line into the **Build Output & Compilation Log** control using thread-safe message posting (`WM_BUILD_LOG_LINE`).
- **Build Backend Choice**: Before every build, Disk Studio asks which environment to use — MSYS2 (Windows-native, no Linux required) or WSL Ubuntu. Your answer is saved and offered as the default next time.
- **Safety Locks**: Action buttons are automatically disabled during compilation to avoid concurrent build conflicts and re-enabled upon completion.

---

## 2. Granular Software Package Checklist & Size Tracking

Disk Studio gives you complete control over which applications and utilities are bundled onto your disk image:

### Base Boot Core Accounting & True Disk Capacity
To ensure a disk actually boots, Disk Studio automatically calculates the **Base OS Core** requirement before adding any software packages:
- **Boot Blocks (`loader.bin`)**: 1,024 bytes (Sectors 0 & 1).
- **PlusOS Kernel (`kernel.pex`)**: ~165 KB (measured dynamically from `build/kernel/kernel.pex`).
- **HFS Volume Structures & Partition Table**: ~24 KB (Master Directory Block, Volume Bitmap, Catalog & Extents B-Trees).
- **System Startup Configuration (`multiboot.cfg`)**: ~1 KB.
- **Base OS Core Requirement**: **~191 KB** (Always required to boot, cannot be omitted).

### Interactive Checklist & Live Sizing
- Displays all **66 built-in applications** categorized across **System**, **Tools**, **Games**, **Development & Emulation**, and **Networking**.
- **Individual File Sizes**: Displays the exact byte/kilobyte size of every binary directly in the list (e.g. `DESKTOP`: 62.2 KB, `PKGMGR`: 30.4 KB, `BASIC`: 22.0 KB, `RET`: 492 B).
- **Live 3-Tier Sizing Breakdown**:
  ```text
  Base OS Core: 191 KB (Kernel, Boot Blocks & HFS - Required)
  Selected Software: 14 of 66 packages (185 KB)
  Total Bootable Footprint: 376 KB (191 KB Base + 185 KB Apps)
  ```

### Floppy Disk Capacity Indicator & Warnings
Macintosh floppy disks have strict physical capacity boundaries. Disk Studio calculates your **Total Bootable Footprint** (Base Core + Software) against these hardware limits:
- **800K Floppy Disks** (Macintosh Plus & SE 800K DSDD Drives): Total bootable size must stay **under 800 KB** (leaving **~609 KB** maximum for software packages).
- **1.44 MB High Density Disks** (Macintosh SE FDHD / SuperDrive): Total bootable size can hold up to **1.44 MB** (leaving **~1.25 MB** maximum for software packages).
- **Visual Status Indicator**:
  - `Floppy: Fits 800K & 1.44M (376 KB / 800 KB | 424 KB Free)`
  - `Floppy: Fits 1.44M HD ONLY (940 KB / 1.44 MB | 500 KB Free - EXCEEDS 800K!)`
  - `Floppy: EXCEEDS 1.44M! (1.62 MB - Hard Drive Only)`
- **Safety Prompts**: When building a floppy image, Disk Studio alerts you with the exact Base Core vs. Software breakdown if your total footprint exceeds 800 KB or 1.44 MB.

### Quick Preset Filters
- **All**: Selects every application in the repository.
- **None**: Clears all selections for custom minimal provisioning.
- **System**: Selects essential operating system binaries and diagnostic tools.
- **Games**: Selects all 13 retro arcade games (Asteroids, Breakout, Dungeon, Flappy, Invaders, Lunar, Mines, Pacman, Puzzle, Racer, Snake, Starflight, Tetris).

### Custom App Importing & Deletion
- **Add Custom App...**: Browse and import any third-party 68000 binary (`.pex` or `.bin`) from your PC. Disk Studio inspects its file size, assigns it to the `Custom` category, and marks it for installation.
- **Remove Selected**: Unchecks built-in software or permanently removes custom imported binaries.

---

## 3. Automatic Compile Directory Scanner

Disk Studio automatically inspects the compilation directory (`build/apps/*.pex`):
- Any newly compiled application not previously in the built-in catalog or `multiboot.cfg` is **automatically detected**.
- The new application is assigned origin `Discovered`, inserted into the checklist with its exact file size, and checked by default.
- Both Disk Studio and `tools/install.py` incorporate the scanner, ensuring you never have to manually edit manifests when authoring new C programs.

---

## 4. Multi-Boot & Dual-Boot System Configurator

Disk Studio makes dual-booting Classic Mac OS and PlusOS seamless:

- **Boot Mode Selector**:
  1. *Direct Boot: PlusOS Command Shell*
  2. *Direct Boot: PlusOS Desktop (Finder UI)*
  3. *Interactive Multi-Boot Menu (with Countdown)*
  4. *Direct Boot: Mac OS (via SCSI Target)*
- **Boot Countdown Timer**: Configure an automated timeout (0–30 seconds) before the default operating system launches automatically.
- **Boot Entry Manager**: Add, reorder (Move Up / Move Down), cycle targets (SCSI IDs 0–7, Shell, Desktop), and delete operating system entries.
- **Visual Theming & Wallpaper**: Choose between Mac OS 7.1 Platinum, Commodore Amiga Workbench, Windows 95, Windows 98, Windows Vista/7 Aero Glass, or Hacker CRT Matrix Rain.
- **Configuration Persistence**: All preferences and package selections are saved to `multiboot.cfg` and remembered across sessions.

---

## 5. Classic Mac OS "Return to PlusOS" Injector

When dual-booting, returning from Classic Mac OS back to PlusOS historically required powering down the machine or modifying BlueSCSI files manually. PlusOS solves this with the **Return to PlusOS** utility:

- **Toolbox Trap `$A888` (`_Restart`) & XPRAM Offset `$78`**:
  `Return to PlusOS` (`APPL` / `PLOS`) is an authentic Classic Mac OS application. When launched inside System 6 or System 7, it writes the PlusOS SCSI target ID into Extended PRAM (XPRAM offset `$78`) and issues the Macintosh Toolbox `_Restart` trap. The hardware restarts and re-enters PlusOS cleanly.
- **One-Click Image Injection**:
  From the **Hard Drive Explorer**, click **"Inject Return to PlusOS"** to seed the utility into `:Desktop:Return to PlusOS` on the current volume, or click **"Inject to External Disk / Floppy..."** to inject it into any external hard drive or floppy disk image on your computer.

---

## 6. Dual-Pane HFS Explorer

The built-in HFS Explorer provides a complete Macintosh file manager on modern Windows:
- **Folder Hierarchy Tree View**: Browse folders (`:bin:`, `:Games:`, `:System:`, `:Desktop:`) on the left pane.
- **Detailed File List**: Inspect file names, 4-character Mac Type (`APPL`, `TEXT`, `pref`, `BTFL`), Creator code (`PLOS`, `ttxt`, `DMGR`), Data Fork size, Resource Fork size, and Catalog Node ID (CNID).
- **Drag-and-Drop Import**: Drag files from Windows Explorer directly into the Disk Studio window. Disk Studio detects appropriate Mac metadata and copies the file into the active HFS directory.
- **Export & Delete**: Extract vintage files from Mac disk images directly to your PC, or remove unneeded files.

---

## 7. Universal Portability & GitHub Repository Auto-Setup

Disk Studio is designed to be 100% portable across any folder, USB flash drive, or secondary disk partition without relying on hardcoded drive letters or user directory structures:

- **Dynamic Path Detection**: When launched, Disk Studio automatically resolves the repository root by checking its executable folder, parent folders, and current working directory for `Makefile` and `kernel/`.
- **Automatic GitHub Repository Download**: If `PlusOSDiskStudio.exe` is placed into an empty folder or a fresh system, it prompts:
  > *"PlusOS source repository was not found in this folder. Would you like Disk Studio to automatically download and initialize the PlusOS build environment here?"*
  Clicking **Yes** automatically clones or downloads and unpacks the entire PlusOS source tree from GitHub (`https://github.com/g91/PlusOS.git`) right in place.
- **Dynamic Path Translation**: Windows drive paths are automatically converted to the format required by the active build backend — MSYS2 format (`/c/Projects/PlusOS`) or WSL format (`/mnt/c/Projects/PlusOS`) — so cross-compiler invocations work correctly from any drive letter.

---

## 8. Universal BlueSCSI Drive Discovery

Older tools assumed external BlueSCSI SD cards were mounted at drive `F:`. Disk Studio now incorporates an automated removable storage scanner:
- Dynamically scans drive letters `D:` through `Z:`.
- Detects BlueSCSI SD card markers such as `BOOTTEST.hda`, `bluescsi.ini`, `HD60_512 PlusOS.hda`, and `NE4.hda`.
- Automatically sets the build target to write directly to the active BlueSCSI drive (`SDCARD=/mnt/<drive> make install`).
- If no SD card is inserted, it falls back to the self-contained local disk image (`images/PlusOS.hda`).

---

## 9. Build Environment Setup & Backend Selection

Compiling Motorola 68000 bare-metal operating systems requires a GCC cross-compiler. Disk Studio includes a one-click **"Auto-Setup Build Environment (Windows-Native)..."** action and remembers your preference across sessions.

### Backend Choice Dialog

Every time you click **Build Hard Drive Image**, **Build Floppy Image**, or **Setup Build Environment**, Disk Studio asks:

> *Which build environment do you want to use?*
> - **Yes — MSYS2**: Windows-native, no Linux subsystem required. Uses `tools\winbuild\msys2\` (portable, set up by setup.ps1) or a system-wide MSYS2 at `C:\msys64`.
> - **No — WSL Ubuntu**: Linux subsystem path. Requires WSL installed (`wsl --install -d Ubuntu`).
> - **Cancel**: Abort without changing anything.

Your selection is saved to `tools\winbuild\build_config.txt` and pre-selected on the next invocation:

```ini
BUILD_BACKEND=msys2
```

### Backend Auto-Detection Priority

On startup, Disk Studio probes for available backends in this order and logs what it finds:

1. **`tools\winbuild\env.txt`** — written by `setup.ps1` after a successful MSYS2 install. Contains `MSYS2_BASH=` and `TOOLCHAIN_DIR=` paths.
2. **Local portable MSYS2** — `tools\winbuild\msys2\usr\bin\bash.exe` (set up by setup.ps1).
3. **System MSYS2** — `C:\msys64\usr\bin\bash.exe` or `C:\msys2\usr\bin\bash.exe`.
4. **WSL Ubuntu** — probed by launching `wsl.exe -d Ubuntu -- uname` with a 5-second timeout.

### MSYS2 Auto-Setup (`setup.ps1`)

Clicking Setup with MSYS2 selected runs `tools\winbuild\setup.ps1` via PowerShell, which:
- Downloads and installs a portable MSYS2 environment into `tools\winbuild\msys2\`
- Installs the m68k-apple-macos cross-compiler toolchain via MSYS2 packages
- Writes `tools\winbuild\env.txt` with the detected paths for future launches

### WSL Setup Path

Clicking Setup with WSL selected runs `tools\winbuild\bootstrap_toolchain.sh` via `wsl.exe -d Ubuntu`, which installs the Retro68 m68k toolchain and all required packages inside the Ubuntu distribution.

### Python Dependencies

Setup also runs `pip install -r requirements.txt` to install:
- `machfs` — Macintosh HFS volume management
- `pyserial` — ROM extraction & DaynaPORT bridge
- `requests` / `urllib3` — package management

---

## 10. Smart MD5 Compiler Ledger

Disk Studio's build pipeline uses a two-phase MD5 hash ledger to avoid recompiling source files that haven't changed, dramatically speeding up incremental builds.

### Phase 1 — Pre-Build Prescan

Before `make` runs, `tools/hashcc.py --prescan` loads `build/.build_ledger.json`, hashes every tracked source file, and **pre-touches** `.o` object files for unchanged sources so `make` skips their compiler invocations entirely. It prints a build plan showing exactly which files will be recompiled and which will be skipped:

```text
  SKIP    kernel/core/proc.c          (hash unchanged)
  SKIP    kernel/drivers/video/video.c (hash unchanged)
  RECOMPILE kernel/services/net.c     (hash changed)
  NEW     apps/mygame/mygame.c        (not in ledger)
```

### Phase 2 — Per-Compile Wrapper

Every individual `CC`/`CXX`/`AS` invocation is wrapped by `tools/hashcc.py`. On each call it:
1. Checks the central ledger for a matching MD5 hash.
2. On a **cache hit**: touches the `.o` file (satisfying `make`) and returns immediately without invoking the compiler.
3. On a **cache miss**: compiles normally, then writes the new hash back into the ledger.

### Central Ledger Format

The ledger lives at `build/.build_ledger.json` (schema v2):

```json
{
  "entries": {
    "kernel/core/proc.c": {
      "hash": "d41d8cd98f00b204e9800998ecf8427e",
      "obj":  "/abs/path/to/build/kernel/core/proc.c.o",
      "deps": ["kernel/core/proc.h", "kernel/plusos.h"],
      "machine": "se"
    }
  }
}
```

Atomic writes (temp-file + `os.replace()`) prevent ledger corruption under parallel `-j` builds.

### Supported Source Types

The ledger tracks `.c`, `.cpp`, `.cxx`, `.cc`, `.S`, `.s`, and `.asm` files for both the cross-compiler and host tools.

### Ledger Commands

```bash
make dump-cache    # Print the full ledger as a human-readable table
make clean-cache   # Delete all .hashcc sidecars and build/.build_ledger.json
```

---

## 11. Boot Hard Drive Image in Mini vMac

The **"Boot Hard Drive Image in Mini vMac (BlueSCSI)..."** button lets you launch the emulator directly from Disk Studio with one click.

### Requirements

The following files must be present before booting:

| File | Location |
|:-----|:---------|
| `Mini vMac (BlueSCSI patched).exe` or `Mini vMac.exe` | `tools/emulator/` |
| `vMac.ROM` (or `Mac Plus.ROM`, `MacSE.ROM`, etc.) | Same folder as the `.exe` |
| A hard drive image (`.hda`) | `images/PlusOS.hda`, `images/BOOTTEST.hda`, or the path in the Image Path field |

### Auto-Detection Logic

1. **Emulator**: Scans `tools/emulator/` for `Mini vMac (BlueSCSI patched).exe` first, then `Mini vMac.exe`.
2. **ROM**: Looks in the emulator's folder first, then `images/`, then the repo root. Accepted filenames: `vMac.ROM`, `Mac Plus.ROM`, `MacPlus.ROM`, `Mac SE.ROM`, `MacSE.ROM`, `vMacSE.ROM`.
   - If the ROM is found outside the emulator folder, Disk Studio offers to copy it there (Mini vMac requires the ROM in its working directory).
3. **Disk Image**: Uses the Image Path field value if set; otherwise auto-detects `images/PlusOS.hda` → `images/BOOTTEST.hda`.

### Conflict Handling

If a Mini vMac window is already open, Disk Studio sends it a close message before launching a new instance so the disk image is not locked by two processes simultaneously.

---

## 12. In-Place Update — Refresh the OS, Keep Your Files

Rebuilding a disk image from scratch wipes everything on it. Once you have been using PlusOS on a real machine — saving documents, editing configs, dropping things on the Desktop — you do not want a kernel upgrade to erase all of that. The **Update** button solves this: it recompiles the kernel and every bundled application and writes them over an **existing** disk or BlueSCSI card **without reinitializing the filesystem**, carrying all of your personal data across untouched.

### Two Buttons, Two Jobs

Disk Studio now presents two primary build actions, stacked at the top of the actions group:

| Button | What it does |
|:-------|:-------------|
| **Compile && Build Hard Drive Image (fresh)** | Formats a brand-new HFS volume from zero. Everything on the target is replaced. Use this for the very first install or a clean wipe. |
| **Update (recompile && refresh, keep my files)** | Recompiles the OS, then overlays the new kernel + software onto your **current** disk. Your files stay exactly where they are. Use this for every upgrade after the first install. |

### What Is Preserved vs. Replaced

An update treats the volume as two layers: **PlusOS system files** (which it replaces) and **your data** (which it preserves). The dividing line is explicit:

- **Replaced on every update** (the operating system):
  - `bin/` — all bundled applications and utilities
  - `Games/`
  - `multiboot.cfg` — the boot menu / startup configuration
  - `.boot`, `.rsrc`, `.hdbuild.json` — boot blocks, resource forks, and the build manifest
  - PlusOS's own Desktop items: **Return to PlusOS** and **Installer**
- **Preserved, untouched** (your data):
  - `home/`, `etc/`, `tmp/`, `documents/` and any other top-level folders you created
  - Every item you placed on the **Desktop** that is not one of PlusOS's own
  - Anything else on the volume that is not a known system path

The kernel itself (`kernel.pex`) and the boot blocks (`loader.bin`) are always refreshed to the freshly compiled versions.

### The Disk's Boot Driver Is Reused

A bootable Macintosh hard drive carries an **Apple SCSI driver** and partition map (the Apple Partition Map / Apple_Driver43 descriptor) that lets the Mac's ROM find and start the volume. An update **reuses the driver and partition map already on your disk**, so the result still boots on the same hardware you have been running — the update never has to re-derive how your particular machine starts.

### Crash-Safe: a Failed Update Can Never Corrupt Your Disk

Updating writes to a **temporary file first**, then atomically moves it into place only after the whole build and verification succeed:

1. Extract the current volume's user data to a temp area.
2. Stage a fresh system layer (new kernel, boot blocks, `bin/`, `Games/`, PlusOS Desktop items).
3. Copy every preserved user file back into the staged layer.
4. Rebuild a complete image in a temp file, reusing your disk's driver + partition map.
5. Verify the image.
6. **Only then** move the temp image over your real disk/card.

If compilation fails, verification fails, or the machine loses power mid-build, your existing disk is still the untouched original — the half-built image lives only in the temp file and is discarded.

### Using It from Disk Studio

1. Insert your PlusOS BlueSCSI SD card (or point the Image Path at a local `.hda`).
2. Click **Update (recompile && refresh, keep my files)**.
3. Confirm the prompt:
   > *This recompiles PlusOS and refreshes the kernel + software on your existing disk. Your documents and settings (home, etc, Desktop items) are preserved, and the disk's Apple SCSI driver is reused. Continue?*
4. Watch the live build log. On success it reports, for example:
   ```text
   preserving user data ...
     preserved 1 user file(s) from the existing volume
   rebuilding image (user files kept, driver reused) ...
   verify: OK
   update complete: kernel + software refreshed, user data preserved.
   Updated: F:\HD60_512 PlusOS.hda
   ```
5. Eject and boot your Mac — same files, new OS.

For LC III and other 68030 machines Disk Studio uses the **universal** build target, so a single updated image boots every supported 680x0 Macintosh. See [Universal Boot](/wiki/19-universal-boot).

### Command-Line Equivalent

The button runs the same pipeline you can invoke by hand through the native toolchain:

```bash
# <target> is the .hda or BlueSCSI card to refresh in place
make.cmd universal update UPDIMG="F:\HD60_512 PlusOS.hda"

# or drive the disk builder directly:
hdbuild update "F:\HD60_512 PlusOS.hda" \
    --build build_p68 \
    --loader build_p68/boot/loader.bin \
    --kernel build_p68/kernel/kernel.pex
```

Pass `--out <path>` (or `OUTIMG=` to the Makefile) to write the updated image to a **new** file instead of overwriting the source — handy for keeping the original card as a fallback.

### Over-the-Internet Updates

The Update button refreshes a disk you physically have in hand. To push updates to machines you cannot reach — fully over the air, with no reflashing — see the [OTA Update Server](/wiki/20-ota-update-server), which serves signed per-compile releases that the on-device PlusOS Updater applies.

---

## 13. Mandatory Core System Lock: `SHELL`

The PlusOS Command Shell (`SHELL`) is the fundamental supervisor runtime environment for the operating system:
- **Permanently Locked**: In the software package checklist, `SHELL` is designated as `Built-in (Required)`.
- **Enforced Across All Presets**: Clicking **None**, **Games**, or **System Only** keeps `SHELL` permanently checked.
- **Protected Against Removal**: Clicking to uncheck `SHELL` or selecting **Remove Selected** immediately prompts that the shell is a mandatory kernel requirement and re-enables it.

---

## 14. Native Linux Edition (`make studio` / `./PlusOSDiskStudio`)

For developers on Linux, Ubuntu, Debian, Arch, or WSL who prefer working natively without Windows:

- **Native C++17 Binary**: Compile directly with `make studio`:
  ```bash
  make studio
  ```
  This builds `./PlusOSDiskStudio_linux` directly with zero external GUI dependencies using native POSIX syscalls (`open`, `pread`, `pwrite`).
- **Universal Root Launcher**: Run `./PlusOSDiskStudio` directly from the repository root:
  ```bash
  chmod +x PlusOSDiskStudio
  ./PlusOSDiskStudio
  ```
- **Interactive TUI & CLI Commands**:
  ```bash
  # Interactive Menu Mode:
  ./PlusOSDiskStudio

  # Inspect HFS disk image partitions and volume stats:
  ./PlusOSDiskStudio info images/PlusOS.hda

  # List all files with sizes, Macintosh FourCC types, and creators:
  ./PlusOSDiskStudio ls images/PlusOS.hda

  # Extract a file data fork from the HFS volume to local Linux disk:
  ./PlusOSDiskStudio extract images/PlusOS.hda "Desktop/HELLO" ./hello_extracted

  # Import a local binary or text file into the HFS volume:
  ./PlusOSDiskStudio import images/PlusOS.hda ./my_app.pex "MYAPP.PEX"

  # Inject Return to PlusOS into any Mac OS image:
  ./PlusOSDiskStudio inject images/MacSystem7.hda 6

  # Run build pipelines natively:
  ./PlusOSDiskStudio build     # Runs make install
  ./PlusOSDiskStudio floppy    # Runs make floppy
  ./PlusOSDiskStudio setup     # Installs requirements.txt dependencies
  ```

---

## Navigation

- **Previous Chapter**: [← 12. SDK & Application Development Guide](/wiki/12-sdk-and-app-development)
- **Next Chapter**: [14. Minimal Bootable Floppy & Hard Drive Installer →](/wiki/14-bootable-floppy-and-installer)
- **Table of Contents**: [Return to Getting Started](/wiki/01-getting-started)

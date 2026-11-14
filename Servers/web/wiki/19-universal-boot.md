# Universal Boot: One Image for Every 680x0 Mac

PlusOS can be built as a single **universal image** that boots on a Macintosh Plus, SE, Classic (68000) **or** an LC III (68030) and chooses the correct behavior at run time — exactly the way System 7 ships one System file that runs on every 680x0 Macintosh. The same `PlusOS.hda` can be moved between a compact Mac and an LC III and it will boot on each, detecting the CPU and hardware at startup.

```bash
make MACHINE=universal install          # one image for all Macs
build universal install                 # Windows front-end
# Disk Studio: Target Machine → "Universal - ONE image, boots any Mac"
```

---

## How System 7 Does It (and How PlusOS Mirrors It)

System 7 does not pick a different build per machine. Instead:

- The ROM / Start Manager detects the processor and records it in the low-memory global **`CPUFlag` at `$012F`** (`0`=68000, `1`=68010, `2`=68020, `3`=68030, `4`=68040). This is the exact test PlusOS uses (`sys71src/OS/StartMgr/Boot3.a → SetUpCPUFlag`).
- The boot blocks are **minimal and processor-independent**; the machine-specific work (caches, video, keyboard) is gated at run time on `CPUFlag` and the ROM/machine type.
- ROM traps abstract the per-machine hardware, so one code path works everywhere.

PlusOS follows the same model: the kernel is compiled `-m68000` (and therefore runs on any 680x0) and branches at run time on `LM_BYTE(0x12F)` and `plusrom_machine()`.

---

## What the Universal Target Builds

| Piece | Behavior |
|---|---|
| **Boot block** (`boot/loader.S`) | The minimal, ROM-independent loader (356 bytes): no Toolbox, finds the boot drive from the drive queue, reads the kernel, checks the `PLOS` magic and jumps. Works on every Mac. |
| **Kernel** (`-m68000`) | One binary. At run time it reads `CPUFlag` and the ROM/machine type and adapts console geometry, framebuffer geometry + back buffer, keyboard, the ROM trap tables, and the A-line dispatch path. |
| **Screen defaults** | Compact-Mac-safe (512×342, 64-byte rows). The LC III's 640×480 Sonora framebuffer is auto-detected at run time. |

### Run-time detection points

| Concern | Where | Test |
|---|---|---|
| Console geometry | `console.c` `con_init` | `CPUFlag ≥ 2` → probe Color QuickDraw; else 512×342 / 64 |
| Framebuffer + back buffer | `video.c` `video_init` | same `CPUFlag` gate |
| 68030 cache flush | `crt0.S`, `aline.S`, `exec.c` | `CPUFlag ≥ 2` emits `movec cacr` |
| ROM trap tables | `plusrom_trap_slot()` | `plusrom_machine()` → `$0C00` (Plus) vs `$0E00` (SE / LC III) |
| Keyboard | `kbd.c` | machine type → ADB vs serial |
| Ring-2 Toolbox gate | `aline.S` | `CPUFlag` → 68000 trampoline vs 68030 direct dispatch |

---

## Why Not One Boot Block With Both Loaders?

An HFS boot block is capped at **1024 bytes** (two 512-byte blocks). The minimal loader is 356 bytes; the LC III QuickDraw "dialog" loader is 992 bytes. Together they exceed 1024, so they cannot share one boot block. The minimal loader already works on every machine, so the universal image uses it everywhere; any machine-specific boot UI belongs in the kernel — which has already detected the machine — not in the 1 KB boot block. This mirrors System 7, whose boot blocks are deliberately tiny and defer everything to the System.

---

## LC III (68030) Run-Time Support

Booting a 68000-compiled kernel on a 68030 requires handling three things the compact Macs never had. PlusOS gates each on `CPUFlag`:

### 1. Cache coherency

The 68030 has on-chip instruction and data caches. Any time PlusOS loads or relocates executable code — in `crt0`, when dispatching a Toolbox trap that may have moved code, and when `exec.c` loads a program image — it must flush the caches so the CPU does not execute stale cached instructions. On a 68030 the kernel emits the privileged `movec` to `CACR`; on a 68000 the same code path is a no-op:

```text
CPUFlag ≥ 2  →  flush/clear I+D caches via  movec … ,cacr
CPUFlag = 0  →  nothing to do
```

### 2. A-Line (Toolbox) dispatch

On the 68000, user-mode Ring-2 programs reach the Toolbox through an A-line trampoline that re-issues the trap in supervisor mode. On the 68030 the kernel takes a **direct-dispatch** path (`aline.S`, gated on `CPUFlag`) and flushes caches around the call, so Toolbox calls from applications behave correctly on the faster, cached CPU. This path is what fixed the LC III "privilege violation when launching the desktop" class of faults.

### 3. Video geometry

The LC III's Sonora controller drives 640×480 at 80 bytes/row instead of the compact Macs' 512×342 at 64. `con_init` and `video_init` probe the real screen dimensions (`ScreenRow` at `LM_WORD(0x82A)`) when `CPUFlag ≥ 2` and adapt the console (80 columns × 60 rows) and the off-screen back buffer automatically; on a 68000 they use the fixed compact-Mac geometry.

For the full reverse-engineering of the 1024 KB LC III ROM that made this port possible, see **[16. Macintosh LC III ROM Reverse-Engineering & Port](/wiki/16-macintosh-lc3-rom-decompilation-and-port)**.

---

## Per-Machine Targets Still Exist

The universal image trades the LC III's fancier boot dialog and native-resolution defaults for one-image portability. When you want a tuned, single-machine build, the dedicated targets remain:

| Target | CPU flags | Notes |
|---|---|---|
| `MACHINE=universal` | `-m68000` | One image, boots any 680x0 Mac |
| `MACHINE=se` | `-m68000` | Macintosh SE / default |
| `MACHINE=plus` | `-m68000` | Serial keyboard translation |
| `MACHINE=classic` | `-m68000` | ADB, 512×342 |
| `MACHINE=lc3` | `-m68030` | Native 640×480, LC III QuickDraw boot dialog |

All five are selectable from the **PlusOS Disk Studio** target menu.

---

## Navigation

- **Previous Chapter**: [← 18. In-Kernel Crash Debugger & Post-Mortem Dumps](/wiki/18-crash-debugger-and-post-mortem-dumps)
- **Next Chapter**: [20. OTA Update Server: Over-the-Air Updates →](/wiki/20-ota-update-server)
- **Table of Contents**: [Return to Getting Started](/wiki/01-getting-started)

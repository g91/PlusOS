# PlusOS web emulator

A from-scratch Macintosh emulator in plain JavaScript (ES modules, no
dependencies, no build step). It runs the real Apple ROM, boots PlusOS (or a
classic System) in the browser and lets people use it with mouse and keyboard.

All code in this directory is original; no source was taken from Mini vMac,
Musashi, UAE, PCE, MAME or any other emulator. References were public hardware
documentation, the documentation in this repository and the behaviour of the
ROMs themselves.

## Run it

```
python emulator/serve.py            # serves the repository root on http://localhost:8068/
```

Open <http://localhost:8068/emulator/> and press **Power on**. The page loads
the ROM and the disk image by relative URL from the repository:

| What | Path |
|------|------|
| Mac Plus ROM | `tools/emulator/vMac.ROM` |
| Mac SE ROM | `ROM/SE/MacSE.ROM`, `ROM/SE/MacSE-stock.ROM` |
| Default disk | `images/PlusOS.hda` (build it with `python tools/hdbuild/hdbuild.py build --out images/PlusOS.hda`) |

ROMs are not copied into `emulator/`. Your own ROM or disk images can be loaded
with the file buttons or by dropping them on the page. If the default disk is
missing the Mac simply shows the blinking "?" disk.

URL parameters (all optional), e.g. `?machine=se&ram=4&disk=../images/PlusOS.hda&autostart=1`:

| Parameter | Values |
|-----------|--------|
| `machine` | `plus`, `se`, `classic` |
| `cpu` | `68000`, `68010`, `68EC020`, `68020`, `68030`, `68EC030`, `68040`, `68LC040`, `68EC040` |
| `ram` | megabytes: `1`, `2`, `4` |
| `rom` | URL of a ROM image |
| `disk` | URL of a disk image; may be repeated; `disk=none` for no disk |
| `scsi=1` | attach the `disk` images to the SCSI bus instead of the built-in driver |
| `speed` | `1`, `2`, `0` (= as fast as possible) |
| `fast=0` | do not skip the ROM's RAM test |
| `mouse=lock` | start in captured-mouse mode |
| `scale` | integer display scale (default: fit) |
| `autostart=1` | power on immediately |
| `embed=1` | show only the screen (for iframes) |
| `worker=0` | run on the main thread instead of a Web Worker |

Controls: model, CPU, RAM, ROM (bundled or uploaded), disks (several, each
either through the built-in driver or SCSI, "Download" returns the image with
the changes made by the Mac), power / reset / interrupt (NMI), pause, speed
1x / 2x / max, sound, full screen, screenshot, mouse mode, on-screen keyboard
button for touch devices, Esc / Tab / Command / Option buttons.
Alt = Option, Windows/Command key = Command; on the Plus, Ctrl also acts as Command.
PRAM is kept in `localStorage` per machine.

## Headless tools (Node)

There is no Node dependency in the product; Node is only used for testing.
Any Node >= 18 works (>= 22 for `browsertest.mjs`).

```
node emulator/tools/headless.mjs --machine plus --disk images/PlusOS.hda --fast --frames 2000 \
     --at 1400:type='root\n' --at 1700:type='Apple\n' --screenshot out.png --trace-traps
node emulator/tools/headless.mjs --machine se --frames 3000 --screenshot se.png
node emulator/tools/run68k.mjs [--cpu 68020] [--base 0x10000] [--ram 4M] [--max-insns N] [--trace] prog.bin
node emulator/tools/browsertest.mjs --url "http://localhost:8068/emulator/?autostart=1" --wait 30 --out page.png
node emulator/tests/cpu/harte.mjs <dir with SingleStepTests 68000 json.gz>
node emulator/tools/run68k.mjs --cpu 68020 emulator/tests/cpu/test020.bin      # exit status 0 = pass
```

* `headless.mjs` runs N video frames, injects keys / mouse (`--at FRAME:ACTION`
  with `type=`, `key=`, `move=`, `mouseto=`, `click`, `shot=`, `nmi`, `reset`),
  writes PNG screenshots (own encoder, zlib from Node) and prints PC,
  exception and A-trap statistics. `--scsi FILE` attaches an image to the SCSI
  bus, `--no-pv` leaves the ROM unpatched, `--cpu` selects the CPU model.
* `run68k.mjs` is the bare CPU harness used by the compiler tests; its
  interface is documented in `tools/RUN68K_READY.txt` and must stay stable.
* `browsertest.mjs` drives headless Chrome/Edge through the DevTools protocol
  and fails if the page logs an error.

## What works

Verified with the commands above (screenshots were inspected):

| Machine | ROM | Status |
|---------|-----|--------|
| Macintosh Plus | `tools/emulator/vMac.ROM` (v3, 4D1F8172) | Boots to the grey desktop with the blinking "?" disk; mouse and keyboard work; boots PlusOS from an Apple-partition-map image and from there to login, shell and desktop. Also runs with 68010 / 68020 / 68030 / 68040 selected. |
| Macintosh SE | `ROM/SE/MacSE.ROM` | Same, with ADB keyboard and mouse. The ROM's SCSI Manager talks to the emulated NCR 5380 (reads complete). |
| Macintosh Classic | none in the repository | Implemented from the machine table as an SE with a 512 KB ROM. **Untested**: supply your own ROM. Its ROM disk (boot from ROM with Cmd-Opt-X-O) and the SWIM chip are not emulated. |
| 512Ke, Classic II, SE/30, II, IIx, IIcx, LC, LC II, LC III, Quadra 700 | - | **Planned only.** Listed in `js/machines/table.js` with CPU, clock and notes; greyed out in the UI. None of their hardware (second VIA, ASC, NuBus/built-in video, Egret, MMU-based memory maps) exists yet. |

Also verified with a real-world image (a BlueSCSI `HD10_512 System 6.0.8.hda`, not part of the
repository): System 6.0.8 boots to the Finder on the Plus and on the SE both
through the built-in driver and through the emulated SCSI bus with the Apple
driver on the disk; pulling down a menu with the mouse works.

Not verified: sound in a real browser was never listened to (the boot beep
arrives in the sample buffer in the headless test, and the page plays it
through WebAudio); touch input and the soft keyboard were not tried on a phone;
the Classic has no ROM to test with.

CPU core (`js/cpu/`):

* 68000: 815,111 of 815,114 non-address-error vectors of the
  SingleStepTests/680x0 set pass (`tests/cpu/harte.mjs`; the three failures
  are two inconsistent ASL.b vectors and one DIVU case). 6,859 vectors are
  skipped as documented deviations (see below). Of the 178,087 vectors that end
  in an address error, about 78 % match completely; the rest differ in details
  of the 14-byte frame (see "Known inaccuracies").
* 68010 / 68020 / 68030 / 68040 and EC/LC variants: full-format addressing
  modes, bit fields, 32/64-bit multiply and divide, CAS/CAS2, CHK2/CMP2,
  PACK/UNPK, MOVEC/MOVES, TRAPcc, RTD, EXTB, LINK.L, BKPT, CALLM/RTM (trap as
  illegal), MOVE16, CINV/CPUSH (no-ops), 68030 PMOVE/PFLUSH/PLOAD/PTEST and
  68040 MMU registers (stored; translation is identity), 68881/68882/68040
  FPU. `tests/cpu/test020.s` checks a selection of these (assembled binary
  `test020.bin` included). There is no test-vector coverage comparable to the
  68000's for these models.

Speed: about 20-30x a real 8 MHz 68000 in Node and Chrome on the development
machine (measured with `headless.mjs` and the page's "Max" speed).

## How the pieces fit

```
index.html, js/ui/main.js      page, controls, canvas, input, WebAudio
js/ui/worker.js, engine.js     real-time loop (Web Worker, or main thread as fallback)
js/machines/table.js           machine definitions (id, CPU, clock, RAM, ROMs, memory map, status)
js/machines/compact.js         Plus / SE / Classic: bus decoding, scheduler, video, VIA wiring, mouse
js/cpu/m68k.js                 CPU state, effective addresses, exceptions, run loop
js/cpu/ops.js                  68000/68010 instructions, 64K-entry dispatch table builder
js/cpu/ops020.js, fpu.js       68020+ instructions, MMU/cache instructions, FPU
js/devices/via.js              6522 VIA (timers, shift register, interrupts)
js/devices/rtc.js              RTC + 256 bytes PRAM on the VIA serial lines
js/devices/scc.js              8530 SCC (register file, DCD interrupts for the Plus mouse)
js/devices/iwm.js              IWM without a drive
js/devices/kbd_plus.js         Plus keyboard protocol (Inquiry / Instant / Model / Test, keypad prefix)
js/devices/adb.js              ADB transceiver, keyboard and mouse (SE / Classic)
js/devices/scsi.js             NCR 5380 with pseudo-DMA + SCSI disk target
js/devices/pvdisk.js           paravirtual disk: ROM patcher and the emulator side of the driver
js/devices/keycodes.js         KeyboardEvent.code / ASCII -> Mac key codes
js/disk/image.js               image detection: Apple Partition Map, DiskCopy 4.2, raw HFS/MFS
rom_patches/pvdisk.s           68k source of the paravirtual driver
```

Timing: the CPU counts 4 clocks per bus cycle plus the internal clocks of
each instruction (within 1 % of the reference cycle counts over the 68000
test set). A frame is 370 lines of 352 CPU clocks (60.15 Hz). The VIA timers
are advanced lazily and the CPU slice ends exactly at the next timer underflow.
VBL (VIA CA1) fires once per frame, the one-second interrupt (CA2) from the
emulated clock. One sound sample is taken per scan line (22.25 kHz).

## Disks

Two ways to attach an image:

1. **Built-in (paravirtual) driver - the default.** At ROM load the emulator
   searches the ROM for the `.Sony` DRVR (driver header followed by the name;
   no fixed offsets, so it works for the Plus, SE and other ROM versions),
   overwrites the beginning of it with the driver in `rom_patches/pvdisk.s`
   (196 bytes) and recomputes the ROM checksum. The ROM then opens this driver
   as unit 4 (refNum -5) exactly like the floppy driver. Every raw HFS/MFS
   volume, and every `Apple_HFS` partition inside a partitioned image, becomes
   a drive (drive numbers 1, 2, ...) in the drive queue; the ROM reads the boot
   blocks from it and boots. No driver is needed on the disk, so images built
   by `tools/hdbuild` boot. With no image attached there is one empty drive,
   which gives the normal "?" disk.

   Port (32-bit big-endian registers at `$F40000`; the address is stored at
   offset 24 of the driver and patched by the emulator):

   | Offset | Dir | Meaning |
   |--------|-----|---------|
   | +0 | W | parameter block pointer (A0 of the driver call) |
   | +4 | W | DCE pointer (A1) |
   | +8 | W | command: 1 open, 2 prime, 3 control, 4 status. Executed when written. |
   | +12 | R | result (OSErr, sign-extended) |
   | +16 | W | drive index 0..n-1 for the next two registers |
   | +20 | R | drive number of that drive, 0 = no such drive |
   | +24 | W | address of a 24-byte block (4 flag bytes + DrvQEl) allocated by the driver; the emulator fills writeProt, diskInPlace (8 = fixed disk), installed, qType = 1, dQDrvSz, dQDrvSz2 |
   | +28 | R | non-zero: return with RTS instead of jIODone (KillIO) |
   | +32 | R | `'PVDK'` |
   | +36 | R | drive number with a pending disk-inserted event, 0 = none (reading consumes it) |

   Prime: the emulator reads `ioTrap` (2 = read, 3 = write), `ioVRefNum`
   (drive), `ioBuffer`, `ioReqCount` from the parameter block and
   `dCtlPosition` from the DCE, copies the data, then stores `ioActCount` and
   advances `dCtlPosition`. Errors: `offLinErr` (-65) for an empty drive,
   `nsDrvErr` (-56), `paramErr` (-50) beyond the end, `wPrErr` (-44).
   Control: 1 KillIO, 5 verify, 6 format, 7 eject, 8 set tag buffer, 9 track
   cache, 65 accRun answer `noErr`; others `controlErr`. Status: 8 returns the
   drive status record; others `statusErr`.

   Re-assemble the driver with
   `m68k-linux-gnu-as -m68000 pvdisk.s -o pvdisk.o && m68k-linux-gnu-objcopy -O binary pvdisk.o pvdisk.bin`
   and paste the bytes into `PVDISK_HEX` in `js/devices/pvdisk.js`. This should
   move to `toolchain/p68-as` once that assembler is ready.

2. **SCSI.** A real NCR 5380 with a disk target (TEST UNIT READY, REQUEST
   SENSE, INQUIRY, READ/WRITE(6)/(10), READ CAPACITY, MODE SENSE/SELECT,
   FORMAT, VERIFY, START/STOP, ...), including the Mac's pseudo-DMA access
   (address bit 9 = DACK). The image must carry an Apple driver partition, as
   on real hardware. Choose "SCSI" in the disk list or `--scsi` in the
   headless tool.

Writes go to the in-memory copy of the image only; "Download" (browser) or
`--save-disk` (headless) return the modified image.

## Known inaccuracies and limits

* **Address/bus error frames.** 68000 address errors produce a well-formed
  14-byte frame, but the stacked PC and status word differ from real silicon
  for jumps to odd addresses, and `-(An)`/`(An)+` long accesses leave the
  address register adjusted differently. 68010+ group-0 frames (formats $8,
  $A, $7) are well-formed but minimal; faulted instructions are restarted, not
  continued. The compact Macs never raise bus errors.
* **Deviations from the SingleStepTests vectors** (deliberate, the vectors come
  from another emulator): ASR with a count larger than the operand size on a
  negative value sets C and X; DIVU/DIVS overflow sets N.
* **No prefetch emulation**; self-modifying code that changes the next
  instruction word behaves differently from a real 68000.
* **Trace on 68020+ (T0)** is approximated; the 68020 `CALLM`/`RTM` trap.
* **MMU**: registers only, no address translation, no ATC. **FPU**: 53-bit
  mantissa (JS doubles) instead of 64; transcendental functions are computed
  directly also on the 68040 (a real 68040 traps them to software); FSAVE /
  FRESTORE use minimal idle frames; FPU exceptions are never raised.
* **Timing** is instruction-level, not bus-cycle-level. 68020+ models use the
  68000 cycle costs, so "Plus + 68020" is a faster CPU only in the sense that
  it understands more instructions.
* **Interrupt priority**: VIA + SCC pending together is presented as level 2.
* **SCSI on the Plus with a driverless image**: the Plus ROM asks the disk for a
  zero-length driver and waits forever in its blind-read loop (the SE ROM
  copes). Images without an Apple driver partition belong on the built-in
  driver; on SCSI they would not boot on real hardware either.
* **The built-in driver imitates what classic Systems expect from `.Sony`**:
  it allocates SonyVars (`$0134`) and queues an idle Time Manager task at
  SonyVars+280, because System 6 patches both at start-up. Other software that
  pokes deeper into the floppy driver's variables will not find real data.
* **AppleTalk**: the SCC reports an idle line so the `.MPP` driver initialises;
  nothing is ever sent or received.
* **Floppy**: the IWM is a stub without a drive. With `--no-pv` / an unpatched
  ROM the Plus waits forever for the floppy driver to register a drive
  (the SE does not), so keep the built-in driver enabled.
* **SCC**: no serial data, no LocalTalk. **Sound**: one sample per scan line
  with the volume from VIA port A; no filter model.
* **Absolute mouse mode** writes the low-memory mouse position (MTemp,
  RawMouse, CrsrNew) like the ROM's own handlers do; captured mode is the real
  hardware path (quadrature through the SCC on the Plus, ADB on the SE) and
  goes through the ROM's mouse scaling.
* **Plus keyboard**: keypad `+ * / =` are sent as keypad codes without the
  M0110A's shift prefix.
* **Fast boot** skips the ROM's RAM test the way the ROM itself allows: mouse
  button held at power-on (Plus), warm-start flag (SE).
* **ADB**: only the keyboard and mouse; service requests are modelled just far
  enough for the SE ROM.

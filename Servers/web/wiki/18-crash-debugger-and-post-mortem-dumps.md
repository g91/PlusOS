# In-Kernel Crash Debugger & Post-Mortem Dumps

When something faults on a classic Macintosh, the ROM normally shows a Sad Mac and halts. PlusOS replaces that dead end with a **crash debugger**: a fault records exactly what happened — the exception, the full processor state, the stack, the machine code at the crash site, and the recent kernel log — then recovers. The dump is written to disk so it can be analysed offline with the `plusdump` viewer, which disassembles the crash site to 68000 assembly.

The components:

| Piece | File | Role |
|---|---|---|
| Exception entry | `kernel/cpu/exc.S` | Captures the full register file at fault time |
| Crash handler | `kernel/cpu/crash.c` | On-screen panel, dump, and recovery |
| Dump writer | `kernel/cpu/crashdump.c` | Writes the `PDMP` post-mortem file |
| CLI viewer | `tools/crashdump/plusdump.py` | Decodes and prints a dump |
| Disassembler | `tools/crashdump/dis68k.py` | 68000 disassembly of the crash site |

---

## The On-Screen Debugger Panel

The moment a fault is caught, before anything that could fault again, the handler draws a panel straight to the console: the exception name, the program counter (resolved to `kernel+offset` or `app+offset`), the status register, the faulting address for bus/address errors, the complete `D0–D7` / `A0–A7` register file, and the offending process:

```text
 ADDRESS ERROR
  PC=0004a3e0  SR=2000
  D0=0000dead D1=00000003 D2=00000000 D3=0004a3e0
  D4=00000000 D5=00000000 D6=00000000 D7=00000000
  A0=0004a3e0 A1=00000003 A2=00000000 A3=00000000
  A4=00000000 A5=0003f900 A6=00040000 A7=0003fe80
  proc 3 'desktop' ring 2  USP=0003fe80
```

Because the panel is console-only — no heap, no QuickDraw beyond hiding the cursor — it renders even when memory is badly corrupted.

---

## Capturing the Processor State

The exception stubs in `exc.S` snapshot the entire register file **before** anything clobbers it, into `exc_regs[15]` (`D0–D7`, `A0–A6`), alongside the already-captured `A7`/`USP`, `PC`, `SR`, and fault address:

```asm
exc_common:
        movem.l %d0-%d7/%a0-%a6, exc_regs   /* full register file at fault time */
        move.l  %sp, exc_ssp
        ...
```

The handler also distinguishes the 68000 7-word bus/address-error frame from the short frame and the 68010+/68030 frame, so `PC`, `SR`, and the access address are read from the right offsets on every supported CPU.

---

## The `PDMP` Dump Format

The dump writer records a structured, self-describing binary file, **`PlusOS CrashDump`**, on the boot volume. It uses only the raw ROM File Manager traps — no heap, no console — and every memory read is bounds-checked against real RAM so the act of dumping the crash site can never itself fault.

A 36-byte header (magic `PDMP`, version, machine, vector, syserr, ticks, SR, PC, fault address, kernel load base, MemTop) is followed by a sequence of tagged chunks:

| Chunk | Contents |
|---|---|
| `REGS` | `D0–D7`, `A0–A6`, `A7`(SSP), USP, PC |
| `PROC` | pid, ring, name, image / kstack / ustack addresses |
| `STAK` | A window of the supervisor stack |
| `CODE` | The machine code around the crash PC (disassembled by the viewer) |
| `FALT` | Memory around the faulting address (bus/address errors) |
| `KLOG` | The tail of the kernel event log |
| `END ` | Terminator |

All multi-byte values are big-endian, matching how the 68000 stores them.

---

## Reading a Dump: `plusdump`

Pull the `PlusOS CrashDump` file off the volume and open it with the cross-platform CLI viewer:

```bash
python3 tools/crashdump/plusdump.py PlusOS_CrashDump      # full report
python3 tools/crashdump/plusdump.py PlusOS_CrashDump --json   # machine-readable
```

It prints the exception, the full register file, the process, a **disassembly of the code around the crash PC** with the faulting instruction marked, an annotated stack dump (values that point into the kernel or the app image are labelled), the memory at the fault address, and the tail of the kernel log:

```text
====================================================================
  PlusOS Crash Dump
====================================================================
  Machine : Mac SE        t=255 ticks (4.3s)
  Fault   : ADDRESS ERROR
  PC      : 0004a3e0  (desktop + 0x4e0)
  Access  : 00000003   <-- faulting address
  Process : pid 3  'desktop'  ring user
--------------------------------------------------------------------
  Registers
   D0=0000dead  D1=00000003  D2=00000000  D3=0004a3e0
   ...
--------------------------------------------------------------------
  Disassembly around PC  (code @ 0004a3a0)
     0004a3da: 2f00           move.l d0,-(a7)
  >> 0004a3e0: 21fc 0000 dead 0003   move.l #$dead,$3.l   <-- fault
     0004a3e8: 4e75           rts
--------------------------------------------------------------------
  Supervisor stack @ 0003fe80
  ...
  Kernel log (tail)
  | [3.85] <6> [desk] entering main loop
====================================================================
```

### The 68000 disassembler

`dis68k.py` is a compact, dependency-free Motorola 68000 disassembler covering the instructions a C compiler and the kernel actually emit — the full `MOVE`/`MOVEA`/`MOVEQ` family, `LEA`/`PEA`, branches and `DBcc`/`Scc`, `ADD`/`SUB`/`AND`/`OR`/`EOR`/`CMP` and their immediate and address forms, `MULU`/`MULS`/`DIVU`/`DIVS`, shifts and rotates, `MOVEM` with correct predecrement register ordering, `LINK`/`UNLK`, `JMP`/`JSR`/`TRAP`, and effective-address decoding for every addressing mode. It is shared by the CLI and exported as JSON for tooling.

---

## Recovery: The Kernel Keeps Running

After the panel and the dump, the handler **recovers** rather than halting (see **[17. Modular Kernel & Self-Healing](/wiki/17-modular-kernel-and-self-healing)**):

- a faulting **Ring-2 application** is terminated and reaped — `init` respawns the shell and the system stays up;
- a faulting **kernel module worker** is flagged for the supervisor to restart;
- a re-entrancy guard ensures that if the panel or the dump themselves fault, the handler still reaches the recovery step instead of double-faulting into a machine reset.

This turns the classic "a crash resets the Mac" behavior into "a crash is logged, dumped, and recovered from," giving PlusOS a post-mortem debugging story closer to a modern OS than to System 7.

---

## Navigation

- **Previous Chapter**: [← 17. Modular Kernel: Drivers, Services & Self-Healing](/wiki/17-modular-kernel-and-self-healing)
- **Next Chapter**: [19. Universal Boot: One Image for Every 680x0 Mac →](/wiki/19-universal-boot)
- **Table of Contents**: [Return to Getting Started](/wiki/01-getting-started)

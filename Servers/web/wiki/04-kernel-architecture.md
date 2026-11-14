# Kernel Core & Dual-Ring Execution

PlusOS implements a robust dual-privilege execution environment on the Motorola 68000 processor, isolating the kernel supervisor from user processes.

---

## Dual-Ring Security Model

| Privilege Level | 68000 Status | Stack Pointer | Components |
|---|---|---|---|
| **Ring 0 (Supervisor)** | `SR` bit 13 = 1 | `SSP` (Supervisor Stack) | Kernel, Drivers, Preemptive Scheduler, Heap Manager, Trap Interceptors |
| **Ring 2 (Userland)** | `SR` bit 13 = 0 | `USP` (User Stack) | Desktop GUI, Command Shell, CLI Utilities, Games, Classic Mac Apps |

In User Mode (`Ring 2`):
- User programs cannot execute privileged instructions (such as `RESET`, `STOP`, `MOVE to SR`, or modifying hardware interrupt masks).
- Crashes, invalid memory accesses (Bus Error / Address Error), and illegal instructions do not bring down the computer; the kernel catches them via exception vectors and gracefully terminates the faulting process.

---

## System Calls: TRAP #1

Userland processes interact with the kernel through the 68000 `TRAP #1` instruction (Exception Vector 33 at address `$0084`):

```c
long plusos_syscall(int sysno, void *arg);
```

1. **Assembly Stub (`sdks/macplus/src/crt0.S`)**:
   - Places the system call number in `D0`.
   - Places the pointer to arguments in `A0`.
   - Executes `TRAP #1`.
2. **Kernel Entry (`kernel/cpu/syscall.c`)**:
   - The CPU automatically saves `SR` and `PC` onto the Supervisor Stack (`SSP`) and switches to supervisor mode.
   - The kernel validates that `sysno` is within range (1 to 63).
   - Verifies the user pointer `arg` lies within valid RAM boundaries using the `security.c` sentinel.
   - Dispatches to the corresponding kernel subsystem handler.
   - Returns the result in register `D0` and executes `RTE` to restore user mode.

---

## Intercepting ROM A-Line Traps (`kernel/cpu/aline.S`)

Every Macintosh ROM call is encoded as an **A-Line instruction** (16-bit opcode beginning with `$A`, such as `$A9F0` for `_LoadSeg` or `$A8D8` for `_FrameRect`). When executed in user mode:
1. The 68000 generates an A-Line exception (Vector 10 at `$0028`).
2. `aline_entry` intercepts the vector:
   - Saves registers `D0-D7/A0-A6`.
   - Checks if caller was in user mode.
   - Copies trap arguments from `USP` to maintain stack frame validity.
   - Calls the actual ROM routine via the Toolbox Trap Dispatcher.
   - Restores registers and returns via `RTE`.

This allows user applications to seamlessly call QuickDraw, OS Utilities, and File Manager routines without needing supervisor privileges.

---

## Modular Design & Fault Isolation

PlusOS is organised as a **modular kernel**: each subsystem is a supervised module (core, driver, or service) that communicates with the others over named pipelines rather than direct calls. A supervisor restarts any module that fails, so the failure of one part does not take down the whole. See **[17. Modular Kernel: Drivers, Services & Self-Healing](/wiki/17-modular-kernel-and-self-healing)**.

When a fault occurs, the exception handler (`kernel/cpu/crash.c`) does not halt the machine — it **recovers**:

| Faulting context | Action |
|---|---|
| **Ring-2 application** (desktop, shell, game) | Terminated and reaped; `init` respawns the shell and the system keeps running |
| **Kernel module worker** | Flagged via `module_mark_failed()`; the supervisor restarts the module |
| **Anything else** | Drops to the recovery shell |

Before recovering, the handler draws an on-screen debugger panel (exception, full register file, PC, faulting process) and writes a post-mortem **`PlusOS CrashDump`** that can be disassembled offline. A re-entrancy guard means that even if the panel or the dump themselves fault, the kernel still reaches the recovery step instead of double-faulting into a reset. See **[18. In-Kernel Crash Debugger & Post-Mortem Dumps](/wiki/18-crash-debugger-and-post-mortem-dumps)**.

---

## Navigation

- **Previous Chapter**: [← 03. Bootloader & Kernel Relocation](/wiki/03-bootloader-and-startup)
- **Next Chapter**: [05. Dynamic RAM Management & Garbage Collection →](/wiki/05-memory-management-and-gc)
- **Table of Contents**: [Return to Getting Started](/wiki/01-getting-started)

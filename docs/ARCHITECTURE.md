# PlusOS architecture

PlusOS is a layered operating system for the Macintosh Plus and SE (68000).
It is built in stages; this document is the target design and notes which
stage each piece lands in.

## Rings

The 68000 has two privilege levels (supervisor, user) and no MMU, so PlusOS
uses three *software* rings mapped onto them:

| Ring | CPU mode | What runs there |
|------|----------|-----------------|
| 0 kernel   | supervisor | scheduler, context switch, interrupt + trap dispatch, physical memory, drivers |
| 1 services | supervisor | file system, window server, network stack — scheduled tasks that may touch hardware and low memory |
| 2 userland | **user mode** | applications. No privileged instructions (no `move to SR`, `RESET`, `STOP`). Reach the system only through the syscall gate and through ROM A-line traps (which auto-enter supervisor). |

Without an MMU, ring 2 cannot be memory-protected: a buggy app can still
overwrite memory. The ring boundary is enforced by CPU privilege (user mode
faults on privileged instructions, caught by the kernel) and by funnelling
services through one gate, not by address-space isolation.

## The syscall gate

Userland calls the OS with `TRAP #1`: service number in D0, argument block
pointer in A0, result in D0. `TRAP` raises a supervisor exception, so the
kernel runs the request in ring 0/1 and `RTE`s back to user mode. A-line ROM
traps (QuickDraw, Memory Manager, ...) also auto-enter supervisor, so apps can
still call the Toolbox directly; privileged or unsafe ROM paths are wrapped as
syscalls instead.

## Processes

Each process has a kernel (supervisor) stack and, if it is a ring-2 app, a
user stack. Context switches happen only in supervisor (inside a syscall or
the timer tick) and save/restore the callee-saved registers and stack pointer.
A user process's user-mode register state lives in the `TRAP`/interrupt frame
on its kernel stack. Scheduling is cooperative first (yield, and blocking
syscalls yield), then preemptive via the vertical-blank timer.

## Boot sequence

1. ROM loads the boot block, which loads the kernel image and jumps to it.
2. **Ring 0** comes up: relocate, BSS, exceptions, ROM subsystems (heap,
   events, File Manager, QuickDraw), physical memory, the scheduler.
3. The kernel starts **ring 1**: the service tasks (file service, later the
   window server and network stack).
4. The kernel launches the first **ring 2** process. Without the desktop that
   is **Terminal** (the shell as a user application); with the desktop it is
   the **Finder**-style shell, and Terminal runs in a window.

## Layout

| Path | Ring | Role |
|------|------|------|
| `kernel/` | 0 | boot, memory, scheduler, syscall gate, interrupt/trap dispatch, console, drivers core |
| `kernel/drivers/` | 0 | device drivers (keyboard, timer, SCSI block, DaynaPORT network) |
| `os/` | 1 | service tasks: file service, window server, network stack |
| `apps/` | 2 | userland programs (terminal, and others), built against an SDK |

## Drivers

A driver is a table of entry points (init, read, write, control, interrupt)
registered with the kernel. Ring-1 services and syscalls call drivers; apps
never touch them directly. Planned: `kbd` (keyboard), `tick` (VBL timer),
`blk` (SCSI block device via the ROM), `net` (DaynaPORT SCSI-Ethernet over the
BlueSCSI's WiFi) — the last testable only on real hardware.

## Stages

1. **Kernel + userland:** scheduler, syscall gate, user-mode processes, the
   shell moved out of the kernel into the Terminal app. *(in progress)*
2. **Desktop:** window server on QuickDraw; windows, menu bar, mouse; Terminal
   in a window.
3. **Networking:** DaynaPORT driver + TCP/IP; reachable from apps.

## Modules and build profiles

Everything above the bare scheduler/trap core is a *module*, registered in
`kernel/core/modules.c` and brought up by `kernel/core/kmodule.c` in five
stages (memory, base, drivers, services, core). A module has a name, type,
stage, optional dependencies (`"video kbd"`) and `init`/`fini` hooks. A module
whose init fails, or whose dependency is not running, is recorded and skipped;
the rest of the system still boots. `lsmod` in the shell shows each module's
state and init time. If the kernel faults, the crash screen names the module
being initialised, or the last syscall.

Drivers also register a `kdriver` table (init/read/write/control/irq) under a
name (`kbd`, `video`, `snd`, `blk`, `tick`) so other code can find a device
without hard-wiring its functions: `kdrv_find("snd")`. Hot paths keep their
direct calls.

Optional modules are chosen at build time (`kernel/core/kconfig.h`):

    make PROFILE=fast install           full | lean | fast | tiny
    make DISABLE="sound shm" install    drop individual modules
    make PROFILE=tiny ENABLE=sound      preset, but keep one
    make modules                        show what a config builds

`sound profiler entropy power pram shm ramdisk scrap macloader stackguard
watchdog` can be left out. A disabled module's source is not linked and its
hooks compile away; its syscall returns -1. The default profile is `full`.

Software safety nets (no MMU, so these detect rather than prevent):

- **Stack guard:** four canary words at the low end of every kernel and user
  stack, checked on each context switch. A smashed user stack kills that
  program; a smashed kernel stack is a kernel panic back to the shell.
- **Watchdog:** the tick counts time since the heartbeat task last ran. A stall
  of 8 s or more is logged (`dmesg`) when the heartbeat next gets the CPU.

`make test-kmodule` runs the host-side unit test of the registry, driver table
and watchdog (`tools/tests/test_kmodule.c`).
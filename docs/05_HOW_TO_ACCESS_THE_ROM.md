# PlusOS: How to Access the Macintosh ROM, Traps & Low Memory

> **Operating System:** PlusOS  
> **Source Modules:** `kernel/aline.S`, `kernel/rominit.c`, `sdks/macplus/include/plusrom/`, `sdks/macse/include/serom/`  
> **Hardware Target:** Macintosh Plus (128 KB ROM) & Macintosh SE (256 KB ROM)  

---

## 1. ROM Memory Mapping & Hardware Base Addresses

On the Macintosh Plus and Macintosh SE, the Read-Only Memory (ROM) is physically mapped into the Motorola 68000 24-bit physical address space starting at **`$00400000`**:

```
+-----------------------------------+ $FFFFFF
| VIA (6522 Versatile Interface)    | $EFE1FE - $EFFFFE
+-----------------------------------+ $DFE100
| IWM / SWIM (Floppy Controller)    | $C00000 - $DFFFFF
+-----------------------------------+ $9FFFFF
| SCC 8530 (Serial Controller)      | $800000 - $9FFFFF
+-----------------------------------+ $5FFFFF
| NCR 5380 (SCSI Controller)        | $580000 - $5FFFFF
+-----------------------------------+ $43FFFF
| Macintosh SE ROM (256 KB)         | $400000 - $43FFFF (SE & SE FDHD)
| Macintosh Plus ROM (128 KB)       | $400000 - $41FFFF (Plus)
+-----------------------------------+ $400000
| Main RAM (Up to 4 Megabytes)      | $000000 - $3FFFFF
+-----------------------------------+ $000000
```

### Locating ROM Base via Low-Memory Globals
At boot, the ROM initializes low-memory pointers that PlusOS can read:
* **`ROMBase` (`$028E`, 4 bytes)**: Base address of the ROM (`$00400000`).
* **`ROMFont0` (`$0980`, 4 bytes)**: Handle to the system font embedded in ROM.
* **`OSTable` (`$0400`, 1024 bytes)**: Dispatch table for OS traps ($A000–$A7FF, 256 entries).
* **`ToolTable` (`$0C00` Plus / `$0E00` SE)**: Dispatch table for Toolbox traps ($A800–$AFFF).

```c
/* In PlusOS kernel: read low-memory globals */
uint32_t rom_base = LM_LONG(0x028E);    /* Always $00400000 */
uint16_t rom_version = LM_WORD(0x028E + 8); /* $0075 on Plus, $0276 on SE */
```

---

## 2. The Line 1010 (EMT 1010) Trap Dispatch Mechanism

The Macintosh operating system relies on Motorola 68000 unimplemented opcode exception **Vector 10 (`$0028`)**, known as the **EMT 1010 A-Line Trap**:

```
Bit: 15  14  13  12  11  10   9   8   7   6   5   4   3   2   1   0
    +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
    | 1 | 0 | 1 | 0 | T |       Trap Number / Flags                 |
    +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
```

### OS Traps vs Toolbox Traps
* **Bit 11 = 0: OS Trap (`$A000 – $A7FF`)**:
  * Used for low-level operating system calls: Memory Manager (`NewPtr`), File Manager (`_Open`), Device Manager (`_Read`), Event Manager (`PostEvent`).
  * **Calling Convention**: Register-based. Parameters passed in `D0`, `D1`, `D2`, `A0`. Return value placed in `D0` and `A0`. Registers `D3–D7` and `A1–A6` are preserved.
  * Bit 10: Auto-pop flag (`1` = Pop return address before dispatching).
  * Bits 9–8: Flag bits (e.g. `CLEAR` memory flag, `SYS` system heap flag).
  * Bits 7–0: 8-bit OS trap index into `OSTable` at `$0400`.
* **Bit 11 = 1: Toolbox Trap (`$A800 – $AFFF`)**:
  * Used for high-level user interface calls: QuickDraw (`LineTo`, `FrameRect`), Window Manager, Menu Manager, Dialog Manager.
  * **Calling Convention**: Stack-based (Pascal calling convention). Parameters are pushed onto the 68000 stack (`A7`) left-to-right. Return values are pushed before arguments. The routine cleans up the stack upon return (`RTS`).
  * Bits 9–0: 10-bit Toolbox trap index into `ToolTable` (`$0C00` or `$0E00`).

---

## 3. The PlusOS A-Line Trap Gate (`kernel/aline.S`)

### The Architectural Problem
The Macintosh ROM was written in 1983–1986 under the assumption that all software runs exclusively in **Supervisor Mode** on a single unified stack. 

In PlusOS, user applications run in **Ring 2 (User Mode)** using the User Stack Pointer (`USP`). When a user application executes an A-line trap (`$Axxx`):
1. The 68000 CPU automatically switches to the Supervisor Stack Pointer (`SSP`).
2. If executed naively, the ROM Toolbox routine would attempt to pop Pascal arguments off the *supervisor* stack (where they don't exist!), corrupting the kernel stack and crashing the system!

### The PlusOS Solution: Trampoline & Stack Bridge (`aline.S`)
PlusOS intercepts Vector 10 (`$0028`) at boot with `aline_entry`:

```assembly
aline_entry:
        btst    #5, (%sp)                   /* Check S-bit in caller's Status Register */
        bne.s   9f                          /* If caller was ALREADY Supervisor -> go straight to ROM */

        /* Caller was in USER MODE: Bridge stacks */
        move.l  %a0, aline_a0               /* Save scratch register */
        movea.l 2(%sp), %a0                 /* %a0 = Program Counter of the $Axxx instruction */
        move.w  (%a0)+, aline_tramp         /* Copy the $Axxx opcode into a dynamic trampoline */
        move.l  %a0, aline_retpc            /* Save resume address after the trap */
        addq.l  #6, %sp                     /* Pop the exception frame (SR + PC) */

        move.l  %sp, aline_ssp              /* Save kernel Supervisor Stack Pointer */
        move.l  %usp, %a0
        movea.l %a0, %sp                    /* Point A7 at the CALLER'S USER STACK! */

        pea     aline_tramp                 /* Set return PC = trampoline */
        move.w  %sr, -(%sp)                 /* Push supervisor SR */
        movea.l aline_a0, %a0
        move.l  aline_orig, -(%sp)
        rts                                 /* Enter ROM dispatcher */

9:      move.l  aline_orig, -(%sp)
        rts                                 /* Directly into ROM */
```

### Trampoline Return (`aline_back`)
When the ROM routine finishes, it executes `RTS`, which jumps to `aline_back`:
```assembly
aline_back:
        move.w  %sr, aline_ccr              /* Capture condition codes set by routine */
        move.l  %sp, aline_usp              /* Save updated user stack pointer */
        movea.l aline_ssp, %sp              /* Switch back to Kernel Supervisor Stack */
        
        move.l  %a0, -(%sp)
        movea.l aline_usp, %a0
        move.l  %a0, %usp                   /* Update USP */
        movea.l (%sp)+, %a0

        move.l  aline_retpc, -(%sp)         /* Push resume PC */
        move.w  aline_ccr, -(%sp)           /* Push saved CCR */
        andi.w  #0x001F, (%sp)              /* Ensure User Mode (S=0, interrupts on) */
        rte                                 /* Return to User Mode! */
```
This elegant bridge enables PlusOS userland applications to invoke any ROM Toolbox or QuickDraw call directly with full argument passing and stack safety.

---

## 4. Reading and Patching ROM Trap Addresses

PlusOS programs can inspect or replace the entry point of any ROM trap using the standard Macintosh trap manipulation routines:

### Inspecting a Trap Address (`GetTrapAddress` / `$A146`)
```c
#include <plusrom/plusrom.h>

/* Read the current execution address of QuickDraw FrameRect ($A8A1) */
UniversalProcPtr addr = GetTrapAddress(0xA8A1);
printf("FrameRect entry address: %08lx\n", (unsigned long)addr);
```

### Patching / Hooking a ROM Trap (`SetTrapAddress` / `$A047`)
PlusOS can intercept any ROM routine by replacing its vector table entry:
```c
UniversalProcPtr g_old_newptr;

/* Custom interceptor for Memory Manager NewPtr */
pascal Ptr MyNewPtrHook(Size size)
{
    printf("Intercepted NewPtr allocation of %ld bytes!\n", size);
    /* Call original routine */
    return ((Ptr (*)(Size))g_old_newptr)(size);
}

void install_patch(void)
{
    g_old_newptr = GetTrapAddress(0xA11E);          /* _NewPtr */
    SetTrapAddress((UniversalProcPtr)MyNewPtrHook, 0xA11E);
}
```

---

## 5. Accessing ROM Resources (`RomMapInsert`)

The Macintosh Plus and SE ROMs contain built-in resource forks holding drivers, font data, packages, and definition procedures:

### ROM Resource Types
* **`'DRVR'`**: Built-in device drivers (`.Sony` floppy, `.Sound`, `.AIn`/`.AOut` serial, `.MPP` AppleTalk).
* **`'PACK'`**: Built-in packages (`PACK 3` Standard File, `PACK 6` Binary-Decimal, `PACK 4/5` SANE).
* **`'WDEF'` / `'CDEF'` / `'MDEF'`**: Standard window, control, and menu definition procedures.
* **`'FONT'`**: Built-in bitmap fonts (Chicago 12, Geneva 9, Monaco 9).

### Enabling ROM Resources in the Resource Chain
To enable the Resource Manager to search ROM resources, the system sets the low-memory global:
```c
LM_BYTE(0x0B9E) = 1;    /* RomMapInsert: 1 = search ROM resources */
```
Once set, calling `GetResource('DRVR', 4)` or `GetResource('FONT', 0)` retrieves the resource directly from ROM silicon without disk access.

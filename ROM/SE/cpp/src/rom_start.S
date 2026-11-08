| ===========================================================================
| rom_start.S -- 256 KB Macintosh SE ROM Binary Header & Reset Vector
| Target: Motorola 68000, 256 KB ROM mapped at 0x00400000 (and 0x0 at reset)
| ===========================================================================
    .text
    .globl _rom_base
    .globl _reset_entry
    .globl _aline_dispatcher
    .globl _start_boot

_rom_base:
    | +0x00: ROM Master Checksum (patched by serom_mgr.py)
    .long   0xB2E362A8

    | +0x04: Initial PC (Reset Vector) -> 0x0040002A
    .long   0x0040002A

    | +0x08: ROM Version ($0276 for Macintosh SE)
    .byte   0x02
    .byte   0x76

    | +0x0A: Restart Entry (JMP _start_boot)
    jmp     _start_boot

    | +0x10: Bad Disk Entry
    jmp     _start_boot

    | +0x16: Reserved
    .long   0x00000000

    | +0x1A: ROM Resource Offset
    .long   0x00030000

    | +0x1E: Eject Entry
    jmp     _start_boot

    | +0x24: Dispatcher Table Offset
    .short  0x0000

    | +0x26: Critical Error / Sad Mac
    jmp     _start_boot

    | +0x2C: Reserved
    .short  0x0000

    | +0x2E: Reserved
    .short  0x0000

    | +0x30: Chip 0 Checksum (High 128 KB)
    .long   0x00B22351
    | +0x34: Chip 1 Checksum (Low 128 KB)
    .long   0x00BF7673
    | +0x38: Reserved
    .long   0x00000000
    | +0x3C: Reserved
    .long   0x00000000

| ===========================================================================
| Offset 0x40: STARTBOOT - First instruction executed on hardware
| ===========================================================================
    .org 0x40
_reset_entry:
_start_boot:
    move.w  #0x2700, %sr            | Disable all interrupts (IPL 7)
    movea.l #0x003FA700, %sp        | Set Initial Stack Pointer (just below video RAM)

    | Zero out the low 2 KB of RAM (vectors & low memory globals)
    movea.l #0x00000000, %a0
    move.w  #0x01FF, %d0            | 512 longs = 2048 bytes
1:  clr.l   (%a0)+
    dbra    %d0, 1b

    | Install A-Line Exception Vector (Vector 10 = Offset 0x0028)
    move.l  #_aline_dispatcher, 0x0028

    | Call C++ ROM bootstrap
    jsr     serom_main_bootstrap

    | Halt loop if bootstrap returns
_halt_loop:
    stop    #0x2700
    bra.s   _halt_loop

| ===========================================================================
| A-Line Trap Dispatcher (Exception Vector 10 at 0x0028)
| Decodes A-line trap word ($A000-$AFFF) and routes to C++ trap dispatcher
| ===========================================================================
_aline_dispatcher:
    movem.l %d0-%d7/%a0-%a6, -(%sp) | Save register context
    move.l  60(%sp), %a0            | A0 = PC where trap occurred
    move.w  -(%a0), %d0             | D0 = A-line trap opcode ($Axxx)

    | Push trap word and stack pointer to C++ dispatcher
    move.w  %d0, -(%sp)
    pea     2(%sp)
    jsr     serom_dispatch_aline
    addq.l  #6, %sp

    movem.l (%sp)+, %d0-%d7/%a0-%a6 | Restore register context
    rte                             | Return from exception

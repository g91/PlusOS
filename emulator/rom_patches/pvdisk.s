| pvdisk.s - paravirtual disk driver for the PlusOS web emulator.
|
| At ROM load time the emulator locates the ROM's ".Sony" DRVR (by its header
| and name) and overwrites the start of it with this driver. The ROM then opens
| it exactly like the floppy driver (unit 4, refNum -5), but every request is
| forwarded to the emulator through a small memory-mapped port, so any raw
| HFS/MFS volume image (or the Apple_HFS partition of a partitioned image)
| appears as a drive in the drive queue without a driver on the disk.
|
| Assemble (GNU as, for now; move to toolchain/p68-as when it is ready):
|   m68k-linux-gnu-as -m68000 pvdisk.s -o pvdisk.o
|   m68k-linux-gnu-objcopy -O binary pvdisk.o pvdisk.bin
| then embed the bytes in js/devices/pvdisk.js (PVDISK_DRIVER).
|
| Port registers (32-bit, big endian) at the address stored in `pvbase`
| (offset 24 of the driver; the emulator patches it per machine):
|   +0   W  parameter block pointer (A0 of the driver call)
|   +4   W  device control entry pointer (A1)
|   +8   W  command: 1 open, 2 prime, 3 control, 4 status - executed when written
|   +12  R  result code of the last command (OSErr, sign extended)
|   +16  W  drive index for the +20 / +24 registers (0 .. n-1)
|   +20  R  drive number of drive [index], 0 = no such drive
|   +24  W  address of a 24-byte block for drive [index]: 4 flag bytes followed
|           by the DrvQEl. The emulator fills in the flags and the size fields.
|   +28  R  non-zero: the last command must return with RTS (KillIO)
|   +32  R  'PVDK' identification
|   +36  R  drive number with a pending disk-inserted event (0 = none); reading consumes it

        .equ    PV_PB, 0
        .equ    PV_DCE, 4
        .equ    PV_CMD, 8
        .equ    PV_RESULT, 12
        .equ    PV_INDEX, 16
        .equ    PV_DRIVE, 20
        .equ    PV_DQE, 24
        .equ    PV_RTS, 28
        .equ    PV_EVENT, 36

        .equ    ioTrap, 6
        .equ    dCtlRefNum, 24
        .equ    JIODone, 0x08FC

        .text
hdr:    .word   0x6F00                  | dNeedLock, dNeedTime, status/control/write/read enable
        .word   30                      | drvrDelay: accRun every 30 ticks (when SystemTask runs)
        .word   0                       | drvrEMask
        .word   0                       | drvrMenu
        .word   open - hdr
        .word   prime - hdr
        .word   control - hdr
        .word   status - hdr
        .word   close - hdr
        .byte   5
        .ascii  ".Sony"
        .even
pvbase: .long   0x00F40000              | +24: port address, patched by the emulator

| Open: tell the emulator, then create one drive queue element per attached volume.
open:   movem.l %d3-%d4/%a2-%a3,-(%sp)
        movea.l pvbase(%pc),%a2
        movea.l %a1,%a3                 | A3 = DCE
        move.l  %a0,PV_PB(%a2)
        move.l  %a1,PV_DCE(%a2)
        moveq   #1,%d0
        move.l  %d0,PV_CMD(%a2)
        moveq   #0,%d3
1:      move.l  %d3,PV_INDEX(%a2)
        move.l  PV_DRIVE(%a2),%d4       | drive number, 0 = no more drives
        beq.s   2f
        moveq   #24,%d0
        .short  0xA71E                  | _NewPtr ,SYS,CLEAR
        move.l  %a0,PV_DQE(%a2)         | emulator fills flags, qType, dQDrvSz, dQDrvSz2
        addq.l  #4,%a0                  | A0 -> DrvQEl
        move.l  %d4,%d0
        swap    %d0                     | drive number in the high word
        move.w  dCtlRefNum(%a3),%d0     | driver refNum in the low word
        .short  0xA04E                  | _AddDrive
        addq.l  #1,%d3
        bra.s   1b
2:      movem.l (%sp)+,%d3-%d4/%a2-%a3
        moveq   #0,%d0
        rts

close:  moveq   #-24,%d0                | closErr: this driver cannot be closed
        rts

prime:  moveq   #2,%d0
        bra.s   call
control: moveq  #3,%d0
        bra.s   call
status: moveq   #4,%d0

| Forward the request; the emulator does all the work while the command register is written.
call:   movem.l %a0-%a2,-(%sp)
        movea.l pvbase(%pc),%a2
        move.l  %a0,PV_PB(%a2)
        move.l  %a1,PV_DCE(%a2)
        move.l  %d0,PV_CMD(%a2)
3:      move.l  PV_EVENT(%a2),%d0       | post pending disk-inserted events
        beq.s   4f
        movea.w #7,%a0                  | diskEvt
        .short  0xA02F                  | _PostEvent
        bra.s   3b
4:      move.l  PV_RESULT(%a2),%d0
        tst.l   PV_RTS(%a2)
        movem.l (%sp)+,%a0-%a2          | (MOVEM does not change the flags)
        bne.s   5f
        btst    #1,ioTrap(%a0)          | immediate call (noQueue bit)?
        bne.s   5f
        move.l  JIODone,-(%sp)          | queued call: finish through IODone (A1 = DCE, D0 = result)
5:      rts

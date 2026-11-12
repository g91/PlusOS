/* return_to_plusos.S - Classic Macintosh application to return to PlusOS.
 *
 * Runs under classic Mac OS (6.x/7.x/8.x) on 68000/68020/68030/68040.
 * Sets the default startup device in XPRAM to SCSI ID 6 (PlusOS)
 * and restarts the Mac into PlusOS.
 */

    .text
    .globl _start

    /* CODE segment 1 header:
     * offset 0: 2 bytes (segOffset = 0)
     * offset 2: 2 bytes (segCount = 1)
     */
    .short  0       /* segOffset */
    .short  1       /* segCount */

_start:
    /* Beep to let the user know Return to PlusOS was launched */
    move.w  #30, -(%sp)
    .short  0xA9C8           /* _SysBeep */

    /* Find drive number for SCSI ID 6 (refNum = -39 = 0xFFD9) */
    move.w  #0xFFD9, %d2     /* target refNum: SCSI ID 6 */
    clr.w   %d3              /* default drive number */

    /* Walk DrvQHdr.qHead at low-memory 0x030A */
    movea.l 0x030A, %a1      /* DrvQHdr.qHead */

scan_drv:
    cmpa.w  #0, %a1
    beq.s   found_drv
    cmp.w   8(%a1), %d2      /* dQRefNum == 0xFFD9? */
    bne.s   next_drv
    move.w  6(%a1), %d3      /* dQDrive */
    bra.s   found_drv

next_drv:
    movea.l (%a1), %a1       /* qLink */
    bra.s   scan_drv

found_drv:
    /* Write XPRAM offset 0x78 (4 bytes: drive_num, refnum) */
    subq.l  #4, %sp
    move.w  %d3, (%sp)
    move.w  %d2, 2(%sp)
    move.l  %sp, %a0         /* buffer pointer */
    move.l  #0x00040078, %d0 /* count=4, offset=0x78 */
    .short  0xA052           /* _WriteXPRam */
    addq.l  #4, %sp

    /* Brief pause (~25 ticks) */
    move.l  #0x00020000, %d1
delay_loop:
    subq.l  #1, %d1
    bne.s   delay_loop

    /* Reboot the Mac directly into PlusOS */
    .short  0xA888           /* _Restart */

    /* Fallback if _Restart is not implemented on ancient ROM */
    .short  0xA9F4           /* _ExitToShell */
    rts

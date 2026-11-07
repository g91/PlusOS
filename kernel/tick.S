/* tick.S - the VBL task entry (called ~60 times a second by the ROM's VBL
 * manager via JSR, at interrupt level, with A0 -> our VBLTask). It saves the
 * caller-saved registers, calls tick_c(), re-arms the task, and returns. */
        .text
        .globl  tick_vbl
tick_vbl:
        movem.l %d0-%d2/%a0-%a1, -(%sp)     /* preserve what C may clobber */
        move.l  %a0, tick_vbltask           /* remember the VBLTask address */
        jsr     tick_c
        movea.l tick_vbltask, %a0
        move.w  #1, 10(%a0)                 /* vblCount (offset 10) = 1: run again next frame */
        movem.l (%sp)+, %d0-%d2/%a0-%a1
        rts

        .bss
        .globl  tick_vbltask
tick_vbltask:
        .space  4

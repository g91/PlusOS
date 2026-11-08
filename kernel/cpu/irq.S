/* irq.S - preemption for ring-2 processes.
 *
 * The 60 Hz tick (and every other VIA interrupt) arrives as a level-1
 * autovector interrupt, vector $64. We wrap the ROM's handler: hand it a frame
 * that returns to irq1_after, let it do all its work (VBL tasks, our tick,
 * the cursor, ...), and then - if the code that was interrupted is a user
 * program whose time slice has run out - switch to the next ready process
 * right here, on that program's kernel stack.
 *
 * Only user-mode code is ever preempted. Kernel code, syscalls and ROM traps
 * run in supervisor mode and are never interrupted into a switch, so the
 * kernel itself stays non-preemptive and needs no locks.
 */
        .text
        .globl  irq1_entry
irq1_entry:
        tst.b   0x12F                       /* CPUFlag: 68010+ frames carry a format word */
        beq.s   1f
        move.w  #0x0064, -(%sp)             /* format 0, vector offset $64 */
1:      pea     irq1_after                  /* the ROM handler RTEs to us ... */
        move.w  %sr, -(%sp)                 /* ... in supervisor, at this interrupt level */
        move.l  irq1_orig, -(%sp)
        rts                                 /* into the ROM's level-1 handler */

irq1_after:
        btst    #5, (%sp)                   /* S bit of the interrupted code's SR */
        bne.s   9f                          /* it was supervisor code: never switch */
        tst.l   want_preempt
        beq.s   9f
        movem.l %d0-%d1/%a0-%a1, -(%sp)     /* C scratch registers */
        jsr     preempt_user                /* yields; returns when we run again */
        movem.l (%sp)+, %d0-%d1/%a0-%a1
9:      rte

        .data
        .align  2
        .globl  irq1_orig
irq1_orig:  .long 0

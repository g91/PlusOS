/* switch.S - 68000 context switch and user-mode entry for PlusOS.
 * Switches happen only in supervisor mode (inside a syscall or the tick). */

        .text

/* void ctx_switch(uint32_t *old_ksp, uint32_t new_ksp)
 *   Save the callee-saved registers of the current task on its kernel stack,
 *   store the resulting SP in *old_ksp, load new_ksp, restore and return into
 *   the next task. Classic cooperative switch; both sides are supervisor.
 *   4(sp)=old_ksp  8(sp)=new_ksp on entry (before we push). */
        .globl  ctx_switch
ctx_switch:
        movem.l %d2-%d7/%a2-%a6, -(%sp)     /* 11 longs of callee-saved state */
        movea.l 48(%sp), %a0                /* a0 = old_ksp  (44 pushed + 4 ret addr) */
        move.l  %sp, (%a0)                  /* *old_ksp = current SP */
        movea.l 52(%sp), %a1                /* a1 = new_ksp */
        move.l  %a1, %sp                    /* switch to the next task's stack */
        movem.l (%sp)+, %d2-%d7/%a2-%a6
        rts                                 /* return into the next task */

/* void ctx_start_user(uint32_t usp, uint32_t entry)
 *   Enter user mode: set the user stack pointer, build an RTE frame with SR=0
 *   (user mode, interrupts enabled) and PC=entry, and RTE. Runs once, in
 *   supervisor, when a freshly created user process is first scheduled.
 *   4(sp)=usp  8(sp)=entry */
        .globl  ctx_start_user
ctx_start_user:
        move.l  4(%sp), %a0                 /* a0 = user stack pointer */
        move.l  8(%sp), %a1                 /* a1 = entry point */
        move.l  %a0, %usp                   /* load USP */
        /* 68000 RTE frame: SR (word) at (sp), PC (long) at (sp+2).
         * 68010/020/030/040 RTE frame appends a 2-byte format/vector word at (sp+6). */
        tst.b   0x12F                       /* CPUFlag: 0=68000 */
        beq.s   1f
        clr.w   -(%sp)                      /* Format 0 ($0000) for 68010+ */
1:      move.l  %a1, -(%sp)                 /* PC = entry */
        clr.w   -(%sp)                      /* SR = 0 (user mode, interrupts on) */
        rte                                 /* drop to user mode at entry */

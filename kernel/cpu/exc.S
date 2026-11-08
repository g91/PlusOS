/* exc.S - CPU exception and system-error entry points.
 *
 * Instead of the ROM's Sad Mac / bomb, a crash in the kernel or in a program
 * records what happened, resets the stack and drops back into the shell.
 * Frames: 68000 bus/address errors (vectors 2,3) are 7 words with the fault
 * address at +2, SR at +8, PC at +10; other 68000 exceptions and every 68020+
 * frame start with SR, PC.
 */
        .text

        .macro  EXC vec
        .globl  exc_vec_\vec
exc_vec_\vec:
        move.w  #\vec, exc_vector
        bra.w   exc_common
        .endm

        EXC 2       /* bus error */
        EXC 3       /* address error */
        EXC 4       /* illegal instruction */
        EXC 5       /* divide by zero */
        EXC 6       /* CHK */
        EXC 7       /* TRAPV */
        EXC 8       /* privilege violation */
        EXC 11      /* line 1111 (F-line) */

exc_common:
        move.l  %sp, exc_ssp
        move.l  %a1, -(%sp)
        move.l  %usp, %a1
        move.l  %a1, exc_usp
        movea.l (%sp)+, %a1
        movea.l %sp, %a0
        clr.l   exc_addr
        tst.b   0x12F                       /* CPUFlag: 0 = 68000 */
        bne.s   2f
        cmpi.w  #3, exc_vector
        bhi.s   1f
        move.l  2(%a0), exc_addr            /* 68000 group 0 frame */
        move.w  8(%a0), exc_sr
        move.l  10(%a0), exc_pc
        bra.s   3f
1:      move.w  (%a0), exc_sr               /* 68000 short frame */
        move.l  2(%a0), exc_pc
        bra.s   3f
2:      move.w  (%a0), exc_sr               /* 68010+: SR, PC, format/vector */
        move.l  2(%a0), exc_pc
3:      movea.l __plusos_load_addr, %sp     /* fresh kernel stack */
        move.w  #0x2000, %sr                /* supervisor, interrupts on */
        jmp     kernel_crash

/* _SysError ($A9C9) replacement: D0.W = error code. The ROM sends fatal
 * errors here - e.g. 12 = unimplemented trap, 25 = out of memory,
 * 28 = stack ran into heap. */
        .globl  syserr_handler
syserr_handler:
        move.w  %d0, exc_syserr
        move.w  #-1, exc_vector
        move.l  (%sp), exc_pc               /* return address of the caller */
        clr.l   exc_addr
        movea.l __plusos_load_addr, %sp
        move.w  #0x2000, %sr
        jmp     kernel_crash

        .bss
        .align  2
        .globl  exc_vector, exc_sr, exc_pc, exc_addr, exc_syserr
exc_vector: .space 2
exc_sr:     .space 2
exc_syserr: .space 2
exc_pc:     .space 4
exc_addr:   .space 4
        .globl  exc_ssp, exc_usp
exc_ssp:    .space 4
exc_usp:    .space 4

/* aline.S - the A-line (ROM trap) gate for ring-2 processes.
 *
 * The Mac ROM assumes everything runs in supervisor mode. When user-mode code
 * executes a trap word ($Axxx), the CPU switches to the supervisor stack, but
 * ROM routines take their Pascal arguments and leave their results on "the"
 * stack (A7), and Toolbox routines return with a plain RTS. Run naively from
 * user mode, a trap therefore reads/writes the kernel stack and returns to the
 * program still in supervisor mode - silently corrupting it.
 *
 * This gate fixes that. Traps from supervisor code go straight to the ROM. For
 * a trap from user mode we:
 *   1. drop the exception frame and remember the kernel stack and resume PC,
 *   2. copy the trap word into a trampoline followed by `jmp aline_back`,
 *   3. point A7 at the caller's user stack and hand the ROM dispatcher a frame
 *      whose PC is the trampoline - so the routine sees its arguments where
 *      the caller pushed them,
 *   4. at aline_back (the routine has returned, in supervisor, A7 = the user
 *      stack with arguments popped / results pushed) store A7 back into USP,
 *      return to the kernel stack and RTE to the caller in user mode with the
 *      condition codes the routine set.
 * ROM traps never reschedule (the scheduler is cooperative and only switches
 * inside syscalls), so a single global save area is enough.
 */
        .text
        .globl  aline_entry
aline_entry:
        btst    #5, (%sp)                   /* S bit of the caller's SR */
        bne.s   9f                          /* supervisor caller: ROM directly */

        move.l  %a0, aline_a0               /* scratch */
        movea.l 2(%sp), %a0                 /* PC of the trap word */
        move.w  (%a0)+, aline_tramp         /* trampoline: the same trap word */
        move.l  %a0, aline_retpc            /* resume after the trap word */
        addq.l  #6, %sp                     /* drop SR + PC */
        tst.b   0x12F                       /* CPUFlag: 68010+ adds a format word */
        beq.s   1f
        addq.l  #2, %sp
        move.l  %d0, aline_d0
        .short  0x4E7A, 0x0002              /* movec cacr,d0 - flush: the */
        ori.w   #0x0808, %d0                /* trampoline is fresh code   */
        .short  0x4E7B, 0x0002              /* movec d0,cacr */
        move.l  aline_d0, %d0
1:      move.l  %sp, aline_ssp              /* kernel stack to come back to */
        move.l  %usp, %a0
        movea.l %a0, %sp                    /* run the routine on the user stack */
        tst.b   0x12F
        beq.s   2f
        move.w  #0x0028, -(%sp)             /* 68010+ frame: format 0, vector $28 */
2:      pea     aline_tramp                 /* "trap PC" = trampoline */
        move.w  %sr, -(%sp)                 /* supervisor SR */
        movea.l aline_a0, %a0
        move.l  aline_orig, -(%sp)
        rts                                 /* into the ROM's A-line dispatcher */

9:      move.l  aline_orig, -(%sp)
        rts

/* The ROM routine returns here (via the trampoline), supervisor, A7 = user stack. */
aline_back:
        move.w  %sr, aline_ccr              /* condition codes the routine set */
        move.l  %sp, aline_usp
        movea.l aline_ssp, %sp              /* back on the kernel stack */
        move.l  %a0, -(%sp)
        movea.l aline_usp, %a0
        move.l  %a0, %usp
        movea.l (%sp)+, %a0
        tst.b   0x12F
        beq.s   1f
        move.w  #0x0028, -(%sp)
1:      move.l  aline_retpc, -(%sp)
        move.w  aline_ccr, -(%sp)
        andi.w  #0x001F, (%sp)              /* user mode, interrupts on, keep CCR */
        rte

        .data
        .align  2
        .globl  aline_orig
aline_tramp:
        .short  0xA000                      /* patched: the trap word */
        .short  0x4EF9                      /* jmp abs.l */
        .long   aline_back
aline_orig:  .long 0
aline_a0:    .long 0
aline_d0:    .long 0
aline_ssp:   .long 0
aline_usp:   .long 0
aline_retpc: .long 0
aline_ccr:   .short 0

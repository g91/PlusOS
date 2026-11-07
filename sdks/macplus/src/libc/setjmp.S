/* setjmp.S - setjmp/longjmp for PlusOS (68000).
 * jmp_buf layout: d2-d7, a2-a7 (12 longs), then return PC. */
        .text
        .globl  setjmp
setjmp:
        movea.l 4(%sp), %a0                 /* env */
        movem.l %d2-%d7/%a2-%a7, (%a0)      /* a7 here = sp at call (ret addr on top) */
        move.l  (%sp), 48(%a0)              /* return PC */
        moveq   #0, %d0
        rts

        .globl  longjmp
longjmp:
        movea.l 4(%sp), %a0                 /* env */
        move.l  8(%sp), %d0                 /* val */
        bne.s   1f
        moveq   #1, %d0                     /* longjmp(env, 0) returns 1 */
1:      movem.l (%a0), %d2-%d7/%a2-%a7      /* restores sp too */
        move.l  48(%a0), (%sp)              /* put the saved PC back as return address */
        rts

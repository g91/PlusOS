/* lmath.S - 32-bit multiply / divide helpers for the 68000 and 68010, which only
 * have 16x16 multiply and 32/16 divide instructions. C calling convention:
 * arguments on the stack, result in d0; d0-d1/a0-a1 are scratch.
 * Part of the P68 runtime library (libp68rt.a). */

        .text
        .even

/* unsigned long __mulsi3(unsigned long a, unsigned long b)   (also correct for signed) */
        .globl  __mulsi3
__mulsi3:
        move.w  4(%sp), %d0             /* a.hi */
        mulu.w  10(%sp), %d0            /* a.hi * b.lo */
        move.w  6(%sp), %d1             /* a.lo */
        mulu.w  8(%sp), %d1             /* a.lo * b.hi */
        add.w   %d1, %d0
        swap    %d0
        clr.w   %d0
        move.w  6(%sp), %d1
        mulu.w  10(%sp), %d1            /* a.lo * b.lo */
        add.l   %d1, %d0
        rts

/* internal: d0 = dividend, d1 = divisor  ->  d0 = quotient, d1 = remainder */
__p68_udivmod:
        move.l  %d2, -(%sp)
        cmp.l   #0x10000, %d1
        bcc.s   3f
        /* divisor fits in 16 bits: two hardware divides (traps on divide by zero) */
        move.l  %d0, %d2
        clr.w   %d2
        swap    %d2                     /* d2 = dividend.hi */
        divu.w  %d1, %d2                /* d2 = rem:quot.hi */
        move.w  %d2, %a0                /* keep quot.hi */
        move.w  %d0, %d2                /* d2 = rem:dividend.lo */
        divu.w  %d1, %d2                /* d2 = rem:quot.lo */
        move.w  %a0, %d0
        swap    %d0
        move.w  %d2, %d0                /* d0 = quotient */
        clr.w   %d2
        swap    %d2
        move.l  %d2, %d1                /* d1 = remainder */
        move.l  (%sp)+, %d2
        rts
3:      /* general case: shift and subtract */
        move.l  %d3, -(%sp)
        moveq   #0, %d2                 /* remainder */
        moveq   #31, %d3
4:      add.l   %d0, %d0
        addx.l  %d2, %d2
        cmp.l   %d1, %d2
        bcs.s   5f
        sub.l   %d1, %d2
        addq.l  #1, %d0
5:      dbra    %d3, 4b
        move.l  %d2, %d1
        move.l  (%sp)+, %d3
        move.l  (%sp)+, %d2
        rts

        .globl  __udivsi3
__udivsi3:
        move.l  4(%sp), %d0
        move.l  8(%sp), %d1
        bra.s   __p68_udivmod

        .globl  __umodsi3
__umodsi3:
        move.l  4(%sp), %d0
        move.l  8(%sp), %d1
        bsr.s   __p68_udivmod
        move.l  %d1, %d0
        rts

/* long __divsi3(long a, long b) : truncates toward zero */
        .globl  __divsi3
__divsi3:
        move.l  4(%sp), %d0
        move.l  8(%sp), %d1
        move.l  %d3, -(%sp)
        moveq   #0, %d3
        tst.l   %d0
        bpl.s   1f
        neg.l   %d0
        not.b   %d3
1:      tst.l   %d1
        bpl.s   2f
        neg.l   %d1
        not.b   %d3
2:      bsr.s   __p68_udivmod
        tst.b   %d3
        beq.s   3f
        neg.l   %d0
3:      move.l  (%sp)+, %d3
        rts

/* long __modsi3(long a, long b) : result has the sign of the dividend */
        .globl  __modsi3
__modsi3:
        move.l  4(%sp), %d0
        move.l  8(%sp), %d1
        move.l  %d3, -(%sp)
        moveq   #0, %d3
        tst.l   %d0
        bpl.s   1f
        neg.l   %d0
        not.b   %d3
1:      tst.l   %d1
        bpl.s   2f
        neg.l   %d1
2:      bsr.w   __p68_udivmod
        move.l  %d1, %d0
        tst.b   %d3
        beq.s   3f
        neg.l   %d0
3:      move.l  (%sp)+, %d3
        rts

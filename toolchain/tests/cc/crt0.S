/* test start-up for run68k: call main, report its result through the exit port */
        .text
        .globl  _start
_start:
        jsr     main
        move.l  %d0, 0x00FF0004
1:      bra.s   1b

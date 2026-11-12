/* vblit.S - fast framebuffer block copy (present) in 68000+ assembly.
 *
 * void video_copy(uint8_t *dst, const uint8_t *src, uint32_t n)
 * Unrolls 64-byte bursts cleanly and copies any <64-byte remainder in longs
 * then bytes so arbitrary screen widths/rowBytes work safely.
 */
        .text
        .globl  video_copy
video_copy:
        movea.l 4(%sp), %a1                 /* dst */
        movea.l 8(%sp), %a0                 /* src */
        move.l  12(%sp), %d1
        beq.s   9f
        move.l  %d1, %d0
        lsr.l   #6, %d0                     /* 64-byte chunks */
        bra.s   2f
1:      move.l  (%a0)+, (%a1)+
        move.l  (%a0)+, (%a1)+
        move.l  (%a0)+, (%a1)+
        move.l  (%a0)+, (%a1)+
        move.l  (%a0)+, (%a1)+
        move.l  (%a0)+, (%a1)+
        move.l  (%a0)+, (%a1)+
        move.l  (%a0)+, (%a1)+
        move.l  (%a0)+, (%a1)+
        move.l  (%a0)+, (%a1)+
        move.l  (%a0)+, (%a1)+
        move.l  (%a0)+, (%a1)+
        move.l  (%a0)+, (%a1)+
        move.l  (%a0)+, (%a1)+
        move.l  (%a0)+, (%a1)+
        move.l  (%a0)+, (%a1)+
2:      dbra    %d0, 1b
        swap    %d0
        subq.w  #1, %d0
        bcs.s   3f
        swap    %d0
        bra.s   1b

3:      andi.w  #63, %d1
        move.w  %d1, %d0
        lsr.w   #2, %d0                     /* 4-byte tail */
        bra.s   5f
4:      move.l  (%a0)+, (%a1)+
5:      dbra    %d0, 4b

        andi.w  #3, %d1                     /* byte tail */
        bra.s   7f
6:      move.b  (%a0)+, (%a1)+
7:      dbra    %d1, 6b
9:      rts

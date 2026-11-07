/* gfx_asm.S - PlusGFX inner loops in 68000 assembly.
 *
 * The 68000 can only move words and longs at even addresses, so each routine
 * moves a single byte if the start is odd, then 16 bytes per step, then mops up
 * the rest a byte at a time. Pointer parity is tested on the low byte of the
 * argument on the stack (big-endian: a long at 4(sp) has its low byte at 7(sp)).
 * Only the C scratch registers d0-d1/a0-a1 are used.
 */
        .text

/* void gfx__fill_bytes(uint8_t *p, uint32_t n, uint32_t pat32)
 * pat32 has the same byte in all four positions. */
        .globl  gfx__fill_bytes
gfx__fill_bytes:
        movea.l 4(%sp), %a0
        move.l  8(%sp), %d1
        beq.s   9f
        move.l  12(%sp), %d0
        btst    #0, 7(%sp)                  /* p odd? */
        beq.s   1f
        move.b  %d0, (%a0)+
        subq.l  #1, %d1
1:      movea.l %d1, %a1                    /* a1 = remaining n */
        lsr.l   #5, %d1                     /* d1 = 32-byte blocks (8 longwords) */
        bra.s   3f
2:      move.l  %d0, (%a0)+                 /* 8 longwords = 32 bytes/iteration:   */
        move.l  %d0, (%a0)+                 /* amortizes the dbra loop overhead so  */
        move.l  %d0, (%a0)+                 /* the fill runs near memory bandwidth. */
        move.l  %d0, (%a0)+
        move.l  %d0, (%a0)+
        move.l  %d0, (%a0)+
        move.l  %d0, (%a0)+
        move.l  %d0, (%a0)+
3:      subq.l  #1, %d1
        bpl.s   2b
        move.l  %a1, %d1
        andi.l  #31, %d1
        bra.s   5f
4:      move.b  %d0, (%a0)+
5:      subq.l  #1, %d1
        bpl.s   4b
9:      rts

/* void gfx__copy_bytes(uint8_t *dst, const uint8_t *src, uint32_t n) */
        .globl  gfx__copy_bytes
gfx__copy_bytes:
        movea.l 4(%sp), %a1                 /* dst */
        movea.l 8(%sp), %a0                 /* src */
        move.b  7(%sp), %d0                 /* parity of dst ... */
        move.b  11(%sp), %d1                /* ... and src */
        eor.b   %d1, %d0
        move.l  12(%sp), %d1                /* n */
        beq.s   8f
        btst    #0, %d0
        bne.s   7f                          /* different parity: bytes only */
        btst    #0, 11(%sp)
        beq.s   1f
        move.b  (%a0)+, (%a1)+              /* both odd: one byte to align */
        subq.l  #1, %d1
1:      move.l  %d1, %d0
        lsr.l   #4, %d0
        bra.s   3f
2:      move.l  (%a0)+, (%a1)+
        move.l  (%a0)+, (%a1)+
        move.l  (%a0)+, (%a1)+
        move.l  (%a0)+, (%a1)+
3:      subq.l  #1, %d0
        bpl.s   2b
        andi.l  #15, %d1
        bra.s   7f
5:      move.b  (%a0)+, (%a1)+
7:      subq.l  #1, %d1
        bpl.s   5b
8:      rts

/* void gfx__mask_bytes(uint8_t *dst, const uint8_t *src, const uint8_t *mask, uint32_t n)
 * dst = (dst & ~mask) | (src & mask), byte by byte; fully transparent bytes are skipped. */
        .globl  gfx__mask_bytes
gfx__mask_bytes:
        movem.l %d2/%a2, -(%sp)
        movea.l 12(%sp), %a1                /* dst */
        movea.l 16(%sp), %a0                /* src */
        movea.l 20(%sp), %a2                /* mask */
        move.l  24(%sp), %d2                /* n */
        bra.s   2f
1:      move.b  (%a2)+, %d0                 /* k */
        beq.s   3f
        move.b  (%a0)+, %d1
        and.b   %d0, %d1                    /* src & k */
        not.b   %d0
        and.b   %d0, (%a1)                  /* dst & ~k */
        or.b    %d1, (%a1)+
        bra.s   2f
3:      addq.l  #1, %a0
        addq.l  #1, %a1
2:      subq.l  #1, %d2
        bpl.s   1b
        movem.l (%sp)+, %d2/%a2
        rts

/* void gfx__xor_bytes(uint8_t *p, uint32_t n)   - invert n bytes */
        .globl  gfx__xor_bytes
gfx__xor_bytes:
        movea.l 4(%sp), %a0
        move.l  8(%sp), %d1
        beq.s   8f
        btst    #0, 7(%sp)
        beq.s   1f
        not.b   (%a0)+
        subq.l  #1, %d1
1:      move.l  %d1, %d0
        lsr.l   #4, %d0
        bra.s   3f
2:      not.l   (%a0)+
        not.l   (%a0)+
        not.l   (%a0)+
        not.l   (%a0)+
3:      subq.l  #1, %d0
        bpl.s   2b
        andi.l  #15, %d1
        bra.s   5f
4:      not.b   (%a0)+
5:      subq.l  #1, %d1
        bpl.s   4b
8:      rts

/* vblit.S - the video driver's frame copy (present), in 68000 assembly.
 *
 * void video_copy(uint8_t *dst, const uint8_t *src, uint32_t n)
 * dst and src are even and n is a multiple of 64 (whole 64-byte scanlines).
 * Unrolled to 16 longwords (64 bytes = 1 full 512px row) per iteration with
 * dbra so the page flip outruns the 60 Hz CRT beam without tearing.
 */
        .text
        .globl  video_copy
video_copy:
        movea.l 4(%sp), %a1                 /* dst */
        movea.l 8(%sp), %a0                 /* src */
        move.l  12(%sp), %d0
        lsr.l   #6, %d0                     /* 64-byte scanlines */
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
        rts

/* hwprobe.S - Safe CPU/VBR/hardware probing (bus-error & F-line protected).
 *
 * Standard bus-error probe blocks: masks interrupts, saves the vector at VBR +
 * probe, and restores SP + vector on fault regardless of 68000/010/020/030/040
 * exception frame size.
 */
        .text

        .globl  hw_get_vbr
hw_get_vbr:
        tst.b   0x12F                   /* CPUFlag: 0=68000 (no VBR) */
        beq.s   1f
        .short  0x4E7A, 0x0801          /* movec %vbr, %d0 */
        rts
1:      moveq   #0, %d0
        rts

/* int hw_probe_read8(uint32_t addr, uint8_t *out) -> 0 ok, -1 bus error */
        .globl  hw_probe_read8
hw_probe_read8:
        move.w  %sr, %d1
        ori.w   #0x0700, %sr            /* disable interrupts during probe */
        bsr     get_vbr_a1
        move.l  %a1, probe_vbr
        move.w  #8, probe_off           /* vector 2: bus error */
        move.l  8(%a1), probe_oldvec
        lea     probe_fault(%pc), %a0
        move.l  %a0, 8(%a1)
        bsr     flush_040_dc
        move.w  %d1, probe_sr
        move.l  %sp, probe_sp

        movea.l 4(%sp), %a0
        move.b  (%a0), %d0              /* may bus-error */

        /* succeeded */
        movea.l probe_vbr, %a1
        move.l  probe_oldvec, 8(%a1)
        bsr     flush_040_dc
        move.w  probe_sr, %sr
        movea.l 8(%sp), %a0
        move.l  %a0, %d1
        beq.s   1f
        move.b  %d0, (%a0)
1:      moveq   #0, %d0
        rts

/* int hw_probe_read32(uint32_t addr, uint32_t *out) -> 0 ok, -1 bus error */
        .globl  hw_probe_read32
hw_probe_read32:
        move.w  %sr, %d1
        ori.w   #0x0700, %sr
        bsr     get_vbr_a1
        move.l  %a1, probe_vbr
        move.w  #8, probe_off
        move.l  8(%a1), probe_oldvec
        lea     probe_fault(%pc), %a0
        move.l  %a0, 8(%a1)
        bsr     flush_040_dc
        move.w  %d1, probe_sr
        move.l  %sp, probe_sp

        movea.l 4(%sp), %a0
        move.l  (%a0), %d0              /* may bus-error */

        movea.l probe_vbr, %a1
        move.l  probe_oldvec, 8(%a1)
        bsr     flush_040_dc
        move.w  probe_sr, %sr
        movea.l 8(%sp), %a0
        move.l  %a0, %d1
        beq.s   2f
        move.l  %d0, (%a0)
2:      moveq   #0, %d0
        rts

/* int hw_probe_fpu_raw(void) -> 0 if FPU present, -1 if F-line traps */
        .globl  hw_probe_fpu_raw
hw_probe_fpu_raw:
        move.w  %sr, %d1
        ori.w   #0x0700, %sr
        bsr     get_vbr_a1
        move.l  %a1, probe_vbr
        move.w  #44, probe_off          /* vector 11: Line-F (0x2C) */
        move.l  44(%a1), probe_oldvec
        lea     probe_fault(%pc), %a0
        move.l  %a0, 44(%a1)
        bsr     flush_040_dc
        move.w  %d1, probe_sr
        move.l  %sp, probe_sp

        .short  0xF280, 0x0000          /* fnop */

        movea.l probe_vbr, %a1
        move.l  probe_oldvec, 44(%a1)
        bsr     flush_040_dc
        move.w  probe_sr, %sr
        moveq   #0, %d0
        rts

probe_fault:
        movea.l probe_sp, %sp
        movea.l probe_vbr, %a1
        moveq   #0, %d0
        move.w  probe_off, %d0
        move.l  probe_oldvec, 0(%a1, %d0.l)
        bsr     flush_040_dc
        move.w  probe_sr, %sr
        moveq   #-1, %d0
        rts

get_vbr_a1:
        tst.b   0x12F
        beq.s   1f
        .short  0x4E7A, 0x0801          /* movec %vbr, %d0 */
        movea.l %d0, %a1
        rts
1:      suba.l  %a1, %a1
        rts

flush_040_dc:
        cmpi.b  #4, 0x12F
        bcs.s   1f
        .short  0x4E71, 0xF478          /* nop; cpusha %dc */
1:      rts

        /* uint8_t hw_probe_cpu_level(void): 0=000, 1=010, 2=020, 3=030, 4=040 */
        .globl  hw_probe_cpu_level
hw_probe_cpu_level:
        move.w  %sr, %d2
        ori.w   #0x0700, %sr
        movem.l %d3/%a2-%a3, -(%sp)
        moveq   #0, %d3
        movea.l %sp, %a3
        move.l  0x10, %a2
        lea     cpu_010_fault(%pc), %a0
        move.l  %a0, 0x10
        .short  0x4E7A, 0x0801          /* movec %vbr, %d0 */
        move.l  %a2, 0x10
        movea.l %d0, %a1
        moveq   #1, %d3
        move.l  16(%a1), %a2
        lea     cpu_020_fault(%pc), %a0
        move.l  %a0, 16(%a1)
        .short  0x49C0                  /* extb.l %d0 */
        moveq   #2, %d3
        .short  0x4E7A, 0x0002          /* movec %cacr, %d0 */
        move.l  %d0, %d1
        bset    #31, %d0                /* 68040 IE bit */
        .short  0x4E7B, 0x0002
        .short  0x4E7A, 0x0002
        btst    #31, %d0
        beq.s   cpu_chk_030
        moveq   #4, %d3
        bra.s   cpu_restore_cacr
cpu_chk_030:
        move.l  %d1, %d0
        bset    #12, %d0                /* 68030 WA bit */
        .short  0x4E7B, 0x0002
        .short  0x4E7A, 0x0002
        btst    #12, %d0
        beq.s   cpu_restore_cacr
        moveq   #3, %d3
cpu_restore_cacr:
        move.l  %d1, %d0
        .short  0x4E7B, 0x0002
cpu_020_fault:
        movea.l %a3, %sp
        move.l  %a2, 16(%a1)
        cmpi.b  #4, %d3
        bcs.s   cpu_exit
        .short  0x4E71, 0xF478          /* nop; cpusha %dc */
        bra.s   cpu_exit
cpu_010_fault:
        movea.l %a3, %sp
        move.l  %a2, 0x10
cpu_exit:
        move.l  %d3, %d0
        movem.l (%sp)+, %d3/%a2-%a3
        move.w  %d2, %sr
        rts

        .bss
        .align  2
probe_vbr:    .space 4
probe_oldvec: .space 4
probe_sp:     .space 4
probe_off:    .space 2
probe_sr:     .space 2

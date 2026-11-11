/* loader.S - PlusOS boot block (HFS volume blocks 0-1, max 1024 bytes).
 *
 * The ROM reads these two blocks, sees version $44xx and JSRs to the entry
 * point very early in boot - before the File Manager, the event queue or an
 * application heap exist. We read the kernel image (raw blocks inside the HFS
 * partition) with the boot drive's own driver, place it just below BufPtr and
 * jump to it. The kernel never returns. Position independent: the ROM loads us
 * into a buffer, so only PC-relative references to our own data.
 *
 *   KERNEL_OFFSET  byte offset of the kernel inside the partition
 *   KERNEL_BYTES   bytes to read (max kernel size)
 *
 * TWO boot paths, selected at build time by the target machine (config.mk sets
 * PLUSOS_MACHINE_LC3 for MACHINE=lc3 and passes it to this file):
 *
 *   Compact 68000 Macs (Plus / SE / Classic) - the DEFAULT path.
 *     Minimal and ROM-independent: no Toolbox, no QuickDraw, no assumption
 *     about what the ROM left in A0. It finds the boot drive from the drive
 *     queue, does one _Read, checks the 'PLOS' magic and jumps. Failures draw
 *     black bars straight to the framebuffer (512x342, 64-byte rows). This is
 *     the long-proven path for the compact Macs.
 *
 *   Macintosh LC III (68030) - PLUSOS_MACHINE_LC3.
 *     A diagnostic dialog drawn with QuickDraw ROM traps (_EraseRect,
 *     _FrameRect, _MoveTo, _DrawString, _HideCursor), the kernel read in
 *     32 KB chunks, and a 68020+ cache flush before the jump. This path uses
 *     the System 7 ROM's boot-time QuickDraw/Toolbox environment, which the
 *     128K/256K compact-Mac ROMs do NOT provide the same way - running it on a
 *     Plus or SE is what regressed booting, so it is gated to the LC III only.
 */
#ifndef KERNEL_OFFSET
#define KERNEL_OFFSET (60000 * 512)
#endif
#ifndef KERNEL_BYTES
#define KERNEL_BYTES  (512 * 512)
#endif

        .text
        .globl  _bb
_bb:
        .word   0x4C4B                  /* +00 bbID 'LK' */
        bra.w   start                   /* +02 bbEntry */
        .word   0x4418                  /* +06 bbVersion: $44 -> ROM executes bbEntry */
        .word   0x0000                  /* +08 bbPageFlags */
        .byte   6
        .ascii  "System"                /* +0A bbSysName */
        .space  9
        .byte   6
        .ascii  "Finder"                /* +1A bbShellName */
        .space  9
        .byte   7
        .ascii  "MacsBug"               /* +2A bbDbg1Name */
        .space  8
        .byte   12
        .ascii  "Disassembler"          /* +3A bbDbg2Name */
        .space  3
        .byte   13
        .ascii  "StartUpScreen"         /* +4A bbScreenName */
        .space  2
        .byte   6
        .ascii  "Finder"                /* +5A bbHelloName */
        .space  9
        .byte   9
        .ascii  "Clipboard"             /* +6A bbScrapName */
        .space  6
        .word   10                      /* +7A bbCntFCBs   max open files */
        .word   20                      /* +7C bbCntEvts   event queue size */
        .long   0x00004300              /* +7E bb128KSHeap */
        .long   0x00008000              /* +82 bb256KSHeap */
        .long   0x00020000              /* +86 bbSysHeapSize (512K+ machines) */
        .long   0                       /* +8A */

#ifdef PLUSOS_MACHINE_LC3
/* =====================================================================
 * Macintosh LC III (68030) boot path - QuickDraw diagnostic dialog.
 * ===================================================================== */
start:
        movem.l %d2-%d7/%a2-%a6, -(%sp)

        /* Save parameter block passed by ROM in A0 / D7 */
        move.l  %a0, %a2

        /* 1. Hide cursor */
        .short  0xA852                  /* _HideCursor */

        /* 2. Draw Diagnostic Dialog Box using QuickDraw ROM traps */
        pea     diag_rect(%pc)
        .short  0xA8A3                  /* _EraseRect */
        pea     diag_rect(%pc)
        .short  0xA8A1                  /* _FrameRect */

        /* Inset rect by 2 pixels for double border */
        pea     inner_rect(%pc)
        .short  0xA8A1                  /* _FrameRect */

        /* Title */
        move.w  #55, -(%sp)
        move.w  #50, -(%sp)
        .short  0xA893                  /* _MoveTo */
        pea     str_title(%pc)
        .short  0xA884                  /* _DrawString */

        /* 3. Identify boot drive (D7) and driver refNum (D6).
         * Prioritize parameter block passed by ROM in A0, then low-mem globals.
         */
        clr.w   %d7
        clr.w   %d6

        move.l  %a2, %d0
        beq.s   try_lomem
        move.w  22(%a2), %d7            /* ioVRefNum from ROM PB */
        move.w  24(%a2), %d6            /* ioRefNum from ROM PB */
        tst.w   %d7
        bne.s   have_drive

try_lomem:
        move.w  0x210, %d7              /* BootDrive */
        move.w  0xB34, %d6              /* BtDskRfn */
        tst.w   %d7
        bne.s   have_drive

try_drvq:
        /* Walk DrvQHdr.qHead ($030A) */
        movea.l 0x30A, %a0
walk_dq:
        move.l  %a0, %d0
        beq.s   try_first_dq
        move.w  6(%a0), %d7
        move.w  8(%a0), %d6
        tst.w   %d7
        bne.s   have_drive
        movea.l (%a0), %a0
        bra.s   walk_dq

try_first_dq:
        movea.l 0x30A, %a0
        move.l  %a0, %d0
        beq.s   have_drive
        move.w  6(%a0), %d7
        move.w  8(%a0), %d6

have_drive:
        /* Display Boot Drive & RefNum */
        move.w  #55, -(%sp)
        move.w  #72, -(%sp)
        .short  0xA893                  /* _MoveTo */
        pea     str_drive(%pc)
        .short  0xA884                  /* _DrawString */
        move.w  %d7, %d0
        bsr     draw_hex16

        pea     str_ref(%pc)
        .short  0xA884                  /* _DrawString */
        move.w  %d6, %d0
        bsr     draw_hex16

        tst.w   %d7
        beq     fail_drive

        move.w  %d7, 0x210
        move.w  %d6, 0xB34

        /* 4. Compute load address: below BufPtr, 16-byte aligned */
        move.l  0x10C, %d0              /* BufPtr */
        sub.l   #KERNEL_BYTES + 0x400, %d0
        andi.l  #0xFFFFFFF0, %d0
        movea.l %d0, %a4

        move.w  #55, -(%sp)
        move.w  #92, -(%sp)
        .short  0xA893                  /* _MoveTo */
        pea     str_load(%pc)
        .short  0xA884                  /* _DrawString */
        move.l  %a4, %d0
        bsr     draw_hex32

        /* 5. Display Reading status */
        move.w  #55, -(%sp)
        move.w  #112, -(%sp)
        .short  0xA893                  /* _MoveTo */
        pea     str_reading(%pc)
        .short  0xA884                  /* _DrawString */

        /* 6. Read kernel in 6 chunks of 32 KB (192 KB total) */
        move.l  %a4, %a3                /* A3 = destination buffer */
        move.l  #KERNEL_OFFSET, %d4     /* D4 = partition byte offset */
        moveq   #0, %d5                 /* D5 = chunk index 0..5 */

read_loop:
        /* Clear IOParamBlock */
        lea     pb(%pc), %a0
        moveq   #12, %d0
pz:     clr.l   (%a0)+
        dbra    %d0, pz

        lea     pb(%pc), %a0
        move.w  %d7, 22(%a0)            /* ioVRefNum = drive number */
        move.w  %d6, 24(%a0)            /* ioRefNum  = driver */
        move.l  %a3, 32(%a0)            /* ioBuffer */
        move.l  #32768, 36(%a0)         /* ioReqCount = 32 KB */
        move.w  #1, 44(%a0)             /* ioPosMode = fsFromStart */
        move.l  %d4, 46(%a0)            /* ioPosOffset */
        .short  0xA002                  /* _Read */

        tst.w   %d0
        bne     fail_read

        adda.l  #32768, %a3             /* advance buffer */
        addi.l  #32768, %d4             /* advance partition offset */
        addq.w  #1, %d5
        cmpi.w  #6, %d5
        blt.s   read_loop

        /* 7. Verify 'PLOS' executable magic */
        move.w  #55, -(%sp)
        move.w  #132, -(%sp)
        .short  0xA893                  /* _MoveTo */
        pea     str_magic(%pc)
        .short  0xA884                  /* _DrawString */
        move.l  4(%a4), %d0
        bsr     draw_hex32

        cmpi.l  #0x504C4F53, 4(%a4)
        bne     fail_magic

        pea     str_ok(%pc)
        .short  0xA884                  /* _DrawString */

        /* 8. Launch notice */
        move.w  #55, -(%sp)
        move.w  #152, -(%sp)
        .short  0xA893                  /* _MoveTo */
        pea     str_launch(%pc)
        .short  0xA884                  /* _DrawString */

        /* 9. 68020/030/040 cache flush */
        cmpi.b  #2, 0x12F               /* CPUFlag: 0=68000, 1=68010, 2=68020, 3=68030 */
        bcs.s   go_kernel
        .short  0x4E7A, 0x0002          /* movec cacr, d0 */
        ori.w   #0x0808, %d0            /* clear instruction & data cache */
        .short  0x4E7B, 0x0002          /* movec d0, cacr */

go_kernel:
        /* Register countdown delay (~50,000 cycles, ~2ms) - no timer interrupt dependency */
        move.l  #50000, %d0
1:      subq.l  #1, %d0
        bne.s   1b

        pea     str_go(%pc)
        .short  0xA884                  /* _DrawString */

        jmp     (%a4)                   /* -> kernel crt0; never returns */

fail_drive:
        move.w  #55, -(%sp)
        move.w  #112, -(%sp)
        .short  0xA893                  /* _MoveTo */
        pea     str_err_drive(%pc)
        .short  0xA884                  /* _DrawString */
        bra.s   hang

fail_read:
        move.w  %d0, -(%sp)
        move.w  #55, -(%sp)
        move.w  #132, -(%sp)
        .short  0xA893                  /* _MoveTo */
        pea     str_err_read(%pc)
        .short  0xA884                  /* _DrawString */
        move.w  (%sp)+, %d0
        bsr.s   draw_hex16
        bra.s   hang

fail_magic:
        pea     str_bad(%pc)
        .short  0xA884                  /* _DrawString */

hang:   bra.s   hang

/* draw_hex16: Formats D0.W as 4 hex ASCII characters and calls _DrawString */
draw_hex16:
        movem.l %d0-%d2/%a0, -(%sp)
        lea     str_hex(%pc), %a0
        move.b  #4, (%a0)+
        moveq   #3, %d1
h16_l:  rol.w   #4, %d0
        move.b  %d0, %d2
        andi.b  #0x0F, %d2
        cmpi.b  #10, %d2
        bcs.s   h16_n
        addi.b  #'A'-10, %d2
        bra.s   h16_s
h16_n:  addi.b  #'0', %d2
h16_s:  move.b  %d2, (%a0)+
        dbra    %d1, h16_l
        pea     str_hex(%pc)
        .short  0xA884                  /* _DrawString */
        movem.l (%sp)+, %d0-%d2/%a0
        rts

/* draw_hex32: Formats D0.L as 8 hex ASCII characters and calls _DrawString */
draw_hex32:
        movem.l %d0-%d2/%a0, -(%sp)
        lea     str_hex(%pc), %a0
        move.b  #8, (%a0)+
        moveq   #7, %d1
h32_l:  rol.l   #4, %d0
        move.b  %d0, %d2
        andi.b  #0x0F, %d2
        cmpi.b  #10, %d2
        bcs.s   h32_n
        addi.b  #'A'-10, %d2
        bra.s   h32_s
h32_n:  addi.b  #'0', %d2
h32_s:  move.b  %d2, (%a0)+
        dbra    %d1, h32_l
        pea     str_hex(%pc)
        .short  0xA884                  /* _DrawString */
        movem.l (%sp)+, %d0-%d2/%a0
        rts

        .even
diag_rect:
        .word   28, 40, 172, 440        /* top, left, bottom, right */
inner_rect:
        .word   30, 42, 170, 438

str_title:
        .byte   34
        .ascii  "PlusOS Bootloader 2.0 (Mac LC III)"
str_drive:
        .byte   14
        .ascii  "Boot Drive:  $"
str_ref:
        .byte   14
        .ascii  "    RefNum:  $"
str_load:
        .byte   14
        .ascii  "Load Addr:   $"
str_reading:
        .byte   26
        .ascii  "Reading Kernel (192 KB)..."
str_magic:
        .byte   14
        .ascii  "Magic Check: $"
str_ok:
        .byte   5
        .ascii  " (OK)"
str_bad:
        .byte   6
        .ascii  " (BAD)"
str_launch:
        .byte   19
        .ascii  "Launching PlusOS..."
str_go:
        .byte   6
        .ascii  " [JMP]"
str_err_drive:
        .byte   25
        .ascii  "ERROR: No drive detected!"
str_err_read:
        .byte   20
        .ascii  "ERROR: Read failed $"

        .even
str_hex:
        .space  12
pb:     .space  52

#else
/* =====================================================================
 * Compact 68000 Macs (Plus / SE / Classic) boot path - minimal, proven.
 * No Toolbox or QuickDraw: works regardless of the boot-time ROM state.
 * ===================================================================== */
start:
        movem.l %d2-%d7/%a2-%a6, -(%sp)

        /* find the boot drive in the drive queue -> driver refNum */
        move.w  0x210, %d7              /* BootDrive (drive number) */
        movea.l 0x30A, %a0              /* DrvQHdr.qHead */
dq:     move.l  %a0, %d0
        beq     fail_drive
        cmp.w   6(%a0), %d7             /* dQDrive */
        beq.s   found
        movea.l (%a0), %a0
        bra.s   dq
found:  move.w  8(%a0), %d6             /* dQRefNum */

        /* load address: below BufPtr, 16-byte aligned */
        move.l  0x10C, %d0              /* BufPtr */
        sub.l   #KERNEL_BYTES + 0x400, %d0
        andi.w  #0xFFF0, %d0
        movea.l %d0, %a4

        /* IOParamBlock on our own (PC-relative) storage */
        lea     pb(%pc), %a0
        moveq   #12, %d0
pz:     clr.l   (%a0)+
        dbra    %d0, pz
        lea     pb(%pc), %a0
        move.w  %d7, 22(%a0)            /* ioVRefNum = drive number */
        move.w  %d6, 24(%a0)            /* ioRefNum  = driver */
        move.l  %a4, 32(%a0)            /* ioBuffer */
        move.l  #KERNEL_BYTES, 36(%a0)  /* ioReqCount */
        move.w  #1, 44(%a0)             /* ioPosMode = fsFromStart */
        move.l  #KERNEL_OFFSET, 46(%a0) /* ioPosOffset */
        .short  0xA002                  /* _Read */
        tst.w   %d0
        bne.s   fail_read

        cmpi.l  #0x504C4F53, 4(%a4)     /* 'PLOS' signature? */
        bne.s   fail_magic
        jmp     (%a4)                   /* -> kernel crt0; never returns */

/* Failure: draw 1, 2 or 3 black bars so the cause is visible, then hang. */
fail_drive: moveq   #1, %d5
        bra.s   bars
fail_read:  moveq   #2, %d5
        bra.s   bars
fail_magic: moveq   #3, %d5
bars:   movea.l 0x824, %a0              /* ScrnBase */
        adda.l  #64 * 150, %a0
b1:     move.w  #64 * 12 / 4 - 1, %d0
b2:     move.l  #0xFFFFFFFF, (%a0)+
        dbra    %d0, b2
        adda.l  #64 * 12, %a0           /* gap */
        subq.w  #1, %d5
        bne.s   b1
hang:   bra.s   hang

        .even
pb:     .space  52

#endif /* PLUSOS_MACHINE_LC3 */

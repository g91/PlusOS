/* loader.S - PlusOS boot block (HFS volume blocks 0-1, max 1024 bytes).
 *
 * The ROM reads these two blocks, sees version $44xx and JSRs to the entry
 * point, early in boot: before the File Manager, the event queue or an
 * application heap exist. We read the kernel image (raw blocks inside the
 * HFS partition) with the boot drive's own driver, put it just below BufPtr
 * (the upper half of RAM is unused at this point - the ROM's boot stack sits
 * at MemTop/2) and jump to it. The kernel never returns.
 *
 * Position independent: the ROM loads us into a buffer, so only PC-relative
 * references to our own data.
 *   KERNEL_OFFSET  byte offset of the kernel inside the partition
 *   KERNEL_BYTES   bytes to read (max kernel size)
 */
#ifndef KERNEL_OFFSET
#define KERNEL_OFFSET (60000 * 512)
#endif
#ifndef KERNEL_BYTES
#define KERNEL_BYTES  (384 * 512)
#endif

        .text
        .globl  _bb
_bb:
        .word   0x4C4B                  /* +00 bbID 'LK' */
        bra.w   start                   /* +02 bbEntry */
        .word   0x4418                  /* +06 bbVersion: $44 -> ROM executes bbEntry */
        .word   0x0000                  /* +08 bbPageFlags */
        .byte   6
        .ascii  "System"                /* +0A bbSysName (unused: we never return) */
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

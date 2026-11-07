/* crt0.S - PlusOS start-up code for the kernel (-DPLUSOS_KERNEL) and apps.
 *
 * The image is linked at address 0. The first thing we do, using only
 * PC-relative addressing, is apply the relocation table that tools/mkexe.py
 * appended to the image: every listed long gets the load address added.
 * After that, absolute addressing works and we can clear BSS and call C.
 *
 * Register use before relocation: only d0-d2 / a0-a1 (apps save d2 first).
 */
#include "plusos/exe.h.inc"

        .section .text.crt0,"ax",@progbits
        .globl  _start
_start:
        bra.w   crt0_entry                  /* +0x00 */
        .ascii  "PLOS"                      /* +0x04 magic */
        .word   PEX_FORMAT                  /* +0x08 */
#ifdef PLUSOS_KERNEL
        .word   PEX_KIND_KERNEL             /* +0x0A */
#else
        .word   PEX_KIND_APP
#endif
        .long   0                           /* +0x0C file_size     (mkexe) */
        .long   0                           /* +0x10 mem_size      (mkexe) */
        .long   0                           /* +0x14 reloc_offset  (mkexe) */
        .long   0                           /* +0x18 reloc_count   (mkexe) */
        .long   PLUSOS_STACK_SIZE           /* +0x1C stack_size */
        .long   0                           /* +0x20 reserved */
        .space  12                          /* +0x24 name          (mkexe) */

crt0_entry:
#ifndef PLUSOS_KERNEL
        /* App: we are called as  int entry(int argc, char **argv, const PlusOSAPI *api).
         * Save every callee-saved register so exit() can unwind from anywhere. */
        movem.l %d2-%d7/%a2-%a6, -(%sp)     /* 44 bytes */
#endif
        /* ---- apply relocations (PC-relative only until this is done) ---- */
        lea     _start(%pc), %a0            /* a0 = load address */
        move.l  0x14(%a0), %d1
        lea     0(%a0,%d1.l), %a1           /* a1 = relocation table */
        move.l  0x18(%a0), %d0              /* d0 = count */
        move.l  %a0, %d2                    /* d2 = load address */
        bra.s   2f
1:      move.l  (%a1)+, %d1
        add.l   %d2, 0(%a0,%d1.l)
2:      subq.l  #1, %d0
        bpl.s   1b

#ifdef PLUSOS_KERNEL
        /* 68020/030 accelerators: we just patched code, flush the caches. Only
         * the kernel does this - it runs in supervisor. Apps run in user mode,
         * where movec would fault; the kernel flushes the cache for them in
         * exec_spawn before the process starts. */
        cmpi.b  #2, 0x12F                   /* CPUFlag: 0=68000 1=68010 2+=68020/030 */
        bcs.s   3f
        .short  0x4E7A, 0x0002              /* movec cacr,d0 */
        ori.w   #0x0808, %d0                /* clear instruction + data cache */
        .short  0x4E7B, 0x0002              /* movec d0,cacr */
3:
#endif
        /* ---- clear BSS (overwrites the relocation table we just used) ---- */
        lea     __bss_start, %a1
        move.l  #__bss_end, %d0
        bra.s   5f
4:      clr.b   (%a1)+
5:      cmpa.l  %d0, %a1
        bcs.s   4b

#ifdef PLUSOS_KERNEL
        /* ---- kernel: own the machine. Stack lives just below the image. ---- */
        movea.l %d2, %sp
        lea     __plusos_a5world, %a5       /* A5 world: (A5) -> QuickDraw globals */
        move.l  %a5, 0x904                  /* CurrentA5 */
        move.l  %d2, __plusos_load_addr
        jsr     __plusos_kernel_start       /* never returns */
6:      bra.s   6b
#else
        /* ---- app: remember how to get back, then call C ---- */
        move.l  %sp, __plusos_exit_sp
        move.l  56(%sp), -(%sp)             /* api  (44 saved + 4 ret + 8) */
        move.l  56(%sp), -(%sp)             /* argv */
        move.l  56(%sp), -(%sp)             /* argc */
        jsr     __plusos_app_start          /* returns main()'s result in d0 */
        .globl  __plusos_app_return
__plusos_app_return:                        /* exit() jumps here with d0 = code */
        movea.l __plusos_exit_sp, %sp
        movem.l (%sp)+, %d2-%d7/%a2-%a6
        rts

        .bss
        .align  2
        .globl  __plusos_exit_sp
__plusos_exit_sp:
        .space  4
#endif

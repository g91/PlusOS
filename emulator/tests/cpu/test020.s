| test020.s - self-checking tests for 68020+ integer instructions and the FPU.
| Assemble: m68k-linux-gnu-as -m68040 test020.s -o t.o && m68k-linux-gnu-objcopy -O binary t.o test020.bin
| Run:      node ../../tools/run68k.mjs --cpu 68020 test020.bin   (exit 0 = pass, else failing test number)
        .macro  fail n
        move.l  #\n,0xFF0004
        .endm
        .macro  expect n, val, reg
        cmp.l   #\val,\reg
        beq.s   1f
        fail    \n
1:
        .endm

start:  lea     data(%pc),%a0
| 1: scaled index + EXTB
        moveq   #2,%d0
        move.l  (0,%a0,%d0.l*4),%d1
        expect  1, 0x33333333, %d1
        move.l  #0x1280,%d0
        extb.l  %d0
        expect  2, 0xFFFFFF80, %d0
| 3: memory indirect post-indexed ([bd,An],Xn,od)
        lea     ptrs(%pc),%a1
        move.l  %a0,4(%a1)              | build the pointer table at run time (position independent)
        moveq   #1,%d0
        move.l  ([4,%a1],%d0.l*4,4),%d1
        expect  3, 0x33333333, %d1
| 4: memory indirect pre-indexed ([bd,An,Xn],od)
        move.l  ([0,%a1,%d0.l*4],8),%d1
        expect  4, 0x33333333, %d1
| 5-7: MULU.L / MULS.L 64-bit, DIVU.L
        move.l  #0x12345678,%d0
        move.l  #0x9ABCDEF0,%d1
        mulu.l  %d0,%d2:%d1
        expect  5, 0x0B00EA4E, %d2
        expect  6, 0x242D2080, %d1
        move.l  #-7,%d0
        move.l  #100000,%d1
        muls.l  %d0,%d1
        expect  7, -700000, %d1
        move.l  #1,%d2
        move.l  #0x00000005,%d1
        move.l  #16,%d0
        divu.l  %d0,%d2:%d1             | 0x1_00000005 / 16
        expect  8, 0x10000000, %d1
        expect  9, 5, %d2
        move.l  #-100,%d1
        moveq   #7,%d0
        divsl.l %d0,%d2:%d1
        expect  10, -14, %d1
        expect  11, -2, %d2
| 12-17: bit fields
        move.l  #0x12345678,%d0
        bfextu  %d0{#8:#12},%d1
        expect  12, 0x345, %d1
        bfexts  %d0{#16:#4},%d1
        expect  13, 5, %d1
        move.l  #0xF0F0F0F0,%d0
        bfexts  %d0{#0:#4},%d1
        expect  14, -1, %d1
        lea     scratch(%pc),%a2
        move.l  #0,(%a2)
        move.l  #0,4(%a2)
        moveq   #-1,%d1
        bfins   %d1,(%a2){#6:#20}
        expect  15, 0x03FFFFC0, (%a2)
        bfffo   (%a2){#0:#32},%d1
        expect  16, 6, %d1
        bfchg   (%a2){#28:#8}
        expect  17, 0x03FFFFCF, (%a2)
        cmp.b   #0xF0,4(%a2)
        beq.s   1f
        fail    18
1:      bfclr   %d0{#28:#8}             | wraps around in a register
        expect  19, 0x00F0F0F0, %d0
        bftst   %d0{#0:#8}
        beq.s   1f
        fail    20
1:
| 21-23: CAS, CAS2
        move.l  #5,(%a2)
        moveq   #5,%d0
        moveq   #9,%d1
        cas.l   %d0,%d1,(%a2)
        bne.s   2f
        cmp.l   #9,(%a2)
        beq.s   1f
2:      fail    21
1:      moveq   #5,%d0
        cas.l   %d0,%d1,(%a2)
        beq.s   2f
        cmp.l   #9,%d0
        beq.s   1f
2:      fail    22
1:
| 23-24: CHK2 / CMP2
        lea     bounds(%pc),%a3
        moveq   #50,%d0
        cmp2.l  (%a3),%d0
        bcs.s   2f
        move.l  #200,%d0
        cmp2.l  (%a3),%d0
        bcs.s   1f
2:      fail    23
1:
| 25: PACK / UNPK
        move.w  #0x0407,%d0
        pack    %d0,%d1,#0
        cmp.b   #0x47,%d1
        beq.s   1f
        fail    25
1:      move.b  #0x92,%d0
        unpk    %d0,%d1,#0x3030
        cmp.w   #0x3932,%d1
        beq.s   1f
        fail    26
1:
| 27: RTD, LINK.L, Bcc.L, TRAPcc (not taken)
        move.l  %sp,%a4
        move.l  #0x11111111,-(%sp)
        bsr.l   rtdsub
        cmp.l   %a4,%sp
        beq.s   1f
        fail    27
1:      link.l  %a6,#-0x10000
        unlk    %a6
        cmp.l   %a4,%sp
        beq.s   1f
        fail    28
1:      moveq   #1,%d0
        trapeq
| 29: MOVEC VBR / MOVE from CCR
        lea     0x400,%a5
        movec   %a5,%vbr
        movec   %vbr,%d0
        expect  29, 0x400, %d0
        sub.l   %a5,%a5
        movec   %a5,%vbr
        move.w  #0x1F,%ccr
        move.w  %ccr,%d0
        cmp.w   #0x1F,%d0
        beq.s   1f
        fail    30
1:
        tst.l   fpuflag(%pc)
        beq     done
| ---- FPU ----
        fmove.l #3,%fp0
        fmove.l #4,%fp1
        fmul.x  %fp0,%fp0
        fmul.x  %fp1,%fp1
        fadd.x  %fp1,%fp0
        fsqrt.x %fp0
        fmove.l %fp0,%d0
        expect  40, 5, %d0
        fmove.d dbl(%pc),%fp2            | 1.5
        fadd.s  sgl(%pc),%fp2            | + 2.25
        fmove.d %fp2,(%a2)
        expect  41, 0x400E0000, (%a2)    | 3.75
        fmove.x %fp2,(%a2)
        expect  42, 0x40000000, (%a2)
        expect  43, 0xF0000000, 4(%a2)
        fmove.x (%a2),%fp3
        fcmp.x  %fp2,%fp3
        fbeq    1f
        fail    44
1:      fmovem.x %fp2-%fp3,-(%sp)
        fmove.l #0,%fp2
        fmovem.x (%sp)+,%fp2-%fp3
        fmul.w  #4,%fp2
        fmove.w %fp2,%d0
        cmp.w   #15,%d0
        beq.s   1f
        fail    45
1:      fmove.l #-7,%fp0
        fabs.x  %fp0
        fneg.x  %fp0
        ftst.x  %fp0
        fblt    1f
        fail    46
1:      fmove.l #1,%fp0
        fdiv.l  #3,%fp0                 | 0.333..
        fmul.l  #3,%fp0
        fmove.l %fp0,%d0                | rounds to 1
        expect  47, 1, %d0
        fmovecr #0,%fp0                 | pi
        fmul.l  #1000,%fp0
        fintrz.x %fp0
        fmove.l %fp0,%d0
        expect  48, 3141, %d0
        fmove.l %fpcr,%d0
        fmove.l #0x10,%fpcr              | round toward zero
        fmove.d dbl2(%pc),%fp0           | 2.75
        fmove.l %fp0,%d0
        expect  49, 2, %d0
        fmove.l #0,%fpcr
        fmove.l %fp0,%d0
        expect  50, 3, %d0
done:   move.l  #0,0xFF0004

rtdsub: rtd     #4

        .balign 4
fpuflag: .long  1
data:   .long   0x11111111, 0x22222222, 0x33333333, 0x44444444
ptrs:   .long   0, 0, 0, 0
bounds: .long   10, 100
dbl:    .double 1.5
dbl2:   .double 2.75
sgl:    .single 2.25
scratch: .space 32

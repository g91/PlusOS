| flags: -m68000
/* block comment
   over several lines */
# line comment
	.text
	.globl	entry, shared
	.weak	maybe
entry:	nop ; nop ; rts		| three statements
shared = entry + 2
	.equ	K1, 10
	.set	K2, K1 * 3 + (4 << 2) - 1
K3 = (K2 & 0xf) | 0x100 ^ 3
	.equiv	K4, K3 % 7
	move.w	#K1, %d0
	move.w	#K2, %d1
	move.l	#K3, %d2
	moveq	#K4, %d3
	move.w	#-K1, %d0
	move.w	#~K1 & 0xffff, %d0
	move.w	#(K1 == 10) & 1, %d0
	move.w	#(K1 != 10) & 1, %d0
	move.w	#(K1 < 11) & (K1 > 9) & (K1 <= 10) & (K1 >= 10) & 1, %d0
	move.w	#K1 / 3, %d0
	move.w	#(K1 && 0) + (K1 || 0), %d0
	move.w	#'A, %d0
	move.w	#'\n, %d0
	move.w	#0b1011 + 017 + 0x1f, %d0
1:	dbra	%d0, 1b
	bra.s	2f
	nop
2:	bne.s	1b
1:	bra.s	1b
	lea	later(%pc), %a0
	move.w	#later - entry, %d0
	move.w	#. - entry, %d0
	.long	later - ., . - entry
	.word	later - entry, fwd_const
fwd_const = 0x55
later:
	.byte	1, 2, 'a, 0x80, -1
	.even
	.word	0x1234, -2
	.hword	7
	.short	8
	.long	0x12345678, entry, later + 4, undefined_sym - 8
	.int	9
	.quad	0x1122334455667788
	.ascii	"abc", "d\n\t\0\\\"\101\x41"
	.asciz	"zero"
	.string	"str"
	.align	2
	.byte	1
	.align	4
	.byte	2
	.balign	8
	.byte	3
	.p2align 2
	.byte	4
	.p2align 3, 0x55
	.byte	5
	.balign	4, 0xaa
	.fill	3, 2, 0x1234
	.fill	2
	.space	5
	.skip	3, 0x77
	.zero	2
	.org	0x200
	.byte	0x99
	.type	entry, @function
	.size	entry, . - entry
	.data
dstart:	.long	dstart, entry, later
	.word	dend - dstart
dend:
	.section .rodata
ro:	.asciz	"read only"
	.section .text.extra, "ax", @progbits
extra:	jsr	entry
	jsr	extra
	rts
	.section .data.extra, "aw"
	.long	extra
	.pushsection .text
	moveq	#1, %d7
	.popsection
	.long	ro
	.previous
	.previous
	.bss
buf:	.space	64
	.lcomm	lbuf, 10
	.comm	cbuf, 20, 4
	.comm	cbuf2, 6
	.local	loc_only
	.text
loc_only: rts
	.ident	"p68 test"
	.file	"directives.s"
	.cfi_startproc
	.cfi_endproc

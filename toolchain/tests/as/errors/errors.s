| flags: -m68000
	.text
ok:	nop
	move.l	%d0			| ERROR missing operand
	move.q	%d0, %d1		| ERROR unknown mnemonic
	add.b	%a0, %d0		| ERROR byte op on address register
	lea	%d0, %a0		| ERROR data register is not a control mode
	moveq	#200, %d0		| ERROR out of range
	move.l	(8,%a0,%d0.w*4), %d1	| ERROR scale on a 68000
	bfextu	%d0{#1:#2}, %d1		| ERROR 68020 instruction
	extb.l	%d0			| ERROR 68020 instruction
	fmove.x	%fp0, %fp1		| ERROR no FPU
	movec	%d0, %vbr		| ERROR 68010 instruction
	move.l	([4,%a0]), %d0		| ERROR 68020 addressing
	bra.s	far			| ERROR byte branch out of range
	.space	200
far:	nop
ok:	nop				| ERROR duplicate label
	.long	undefined_a - undefined_b	| ERROR cannot relocate
	.bogus	1			| ERROR unknown directive
	.if	1
	.else
	.else				| ERROR duplicate else
	.endif
	.endif				| ERROR endif without if
	move.w	#1 +, %d0		| ERROR bad expression
	.align	3			| ERROR not a power of two
	.macro	m a:req
	.endm
	m				| ERROR missing required argument
	.error	"explicit"		| ERROR
	asl.w	#9, %d0			| ERROR shift count
	link	%a6, #-70000		| ERROR needs link.l
	trap	#16			| ERROR vector
	bra.s	1f			| ERROR zero byte offset
1:	rts

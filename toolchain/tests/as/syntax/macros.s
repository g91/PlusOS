| flags: -m68000
	.macro	push reg
	move.l	\reg, -(%sp)
	.endm
	.macro	pop reg
	move.l	(%sp)+, \reg
	.endm
	.macro	load val=5, reg=%d0
	move.w	#\val, \reg
	.endm
	.macro	table first, rest:vararg
	.long	\first
	.ifnb	\rest
	table	\rest
	.endif
	.endm
	.macro	labelled n
lbl\n\()x: .byte \n
lbl_\@:	.byte	\@
	.endm
	.macro	early a
	.if	\a
	.exitm
	.endif
	.word	0xdead
	.endm
	.text
start:	push	%d0
	push	(%a0)
	pop	%d1
	load
	load	7
	load	reg=%d3
	load	9, %d4
	table	1, 2, 3, start
	labelled 1
	labelled 2
	early	1
	early	0
	.rept	3
	nop
	.endr
	.irp	r, %d0, %d1, %a0
	clr.l	\r
	.endr
	.irpc	c, 123
	.byte	\c
	.endr
	.rept	2
	.irp	x, 4, 5
	.word	\x
	.endr
	.endr
	.equ	FLAG, 1
	.if	FLAG
	.byte	1
	.else
	.byte	2
	.endif
	.ifdef	FLAG
	.byte	3
	.endif
	.ifndef	FLAG
	.byte	4
	.else
	.byte	5
	.endif
	.ifeq	FLAG - 1
	.byte	6
	.endif
	.ifne	FLAG - 1
	.byte	7
	.endif
	.if	FLAG == 2
	.byte	8
	.elseif	FLAG == 1
	.byte	9
	.else
	.byte	10
	.endif
	.if	0
	.if	1
	.byte	11
	.endif
	this is not assembled
	.else
	.byte	12
	.endif
	.ifgt	FLAG
	.byte	13
	.endif
	.ifc	abc, abc
	.byte	14
	.endif
	.ifnc	abc, abd
	.byte	15
	.endif
	.ifdef	NOTDEFINED
	.error	"must not be seen"
	.endif
	.even
	rts

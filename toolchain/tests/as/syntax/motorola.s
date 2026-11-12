; flags: -m68000 --motorola
; gas-equivalent: motorola.gas
* Motorola / Devpac style source
SIZE	equ	16
COUNT	set	3
COUNT	set	COUNT+1
	rsreset
f_next	rs.l	1
f_flag	rs.b	1
f_val	rs.w	2
f_size	rs.b	0
push	macro
	move.l	\1,-(sp)
	endm
add3	macro
	add.\0	\1,\2
	add.\0	\1,\2
	endm
	xdef	start
	xref	external
	section	text,code
start:	move.l	#$12345678,d0	; hex
	move.w	#%1010,d1	; binary
	move.b	#'A',d2
	moveq	#SIZE,d3
	moveq	#COUNT,d4
	move.l	f_val(a0),d5
	move.w	#f_size,d6
	lea	table(pc),a0
	move.l	(a0)+,d0
	move.l	d0,-(a7)
	move.w	4(a0,d1.w),d2
	move.w	(a1,a2.l),d2
	push	d0
	push	(a0)
	add3.w	d0,d1
	add3.l	#4,a0
.loop	subq.w	#1,d0
	bne.s	.loop
	bsr	sub
	jsr	external
	movem.l	d0-d7/a0-a6,-(sp)
	movem.l	(sp)+,d0-d7/a0-a6
	lea.l	start,a1
	btst.b	#3,(a0)
	rts
sub	clr.l	d0
.loop	dbra	d1,.loop
	rts
	ifd	SIZE
	dc.b	1
	endc
	ifnd	SIZE
	dc.b	2
	endc
	ifeq	SIZE-16
	dc.b	3
	else
	dc.b	4
	endc
	if	SIZE>8
	dc.b	5
	endif
	rept	2
	dc.b	$AA
	endr
	even
table	dc.l	start,sub,*
	dc.w	1,2,-1,$FFFF
	dc.b	'text',0,"more",10
	even
	dcb.b	3,$11
	dcb.w	2,$2222
	ds.b	3
	cnop	0,4
	ds.w	2
	ds.l	1
	section	data,data
dat	dc.l	table
	section	bss,bss
buf	ds.b	100
	end
	this is ignored

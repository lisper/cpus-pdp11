	.text
	.even

/*.=177300^. */

start:
	mov	msg,r0
loop:
	movb	(r0)+,r1	/* get char */
	beq	done		/* if done */
	jsr	pc,tpchr
	br	loop

done:
	halt

tpchr:
	tstb	@tps
	bpl	tpchr
	movb	r1,@tpb
	rts	pc

tpb:
	.word 177566
tps:
	.word 177564

msg:
	.asciz "\r\nHello world!\r\n"


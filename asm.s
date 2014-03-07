	.fopt		compiler,"cc65 v 2.14.0"
	.setcpu		"6502"
	.smart		on
	.importzp	sp
	.import		_mouse_x
	.import		_mouse_dir
	.import		_mouse_frame
	.import		_shift_bits
	.import		_mask_bits
	.import		incsp4
	.export		_get_mouse_ptr
	.export		_set_sprite_pos

.segment        "CODE"

.proc   _get_mouse_ptr: near

        lda     _mouse_dir
        beq     stand
        cmp     #$FE
        beq     spin
	
walk:	lda     _mouse_x
	clc
        adc     _mouse_frame
        lsr     a
        lsr     a
        and     #$07
	clc
        adc     #$08
	rts

stand:	lda     #$10
        rts
	
spin:	lda     _mouse_x
        lsr     a
        lsr     a
        lsr     a
        and     #$07
	clc
        adc     #$10
        rts
	
.endproc

.proc   _set_sprite_pos: near

.segment        "CODE"

        ldy     #$02
	lda     (sp),y
	beq	hi_bit_0
hi_bit_1:
	ldy     #$03
        lda     (sp),y
        tay
        lda     _shift_bits,y
	ora	$D010
	jmp	hi_bit_ok
hi_bit_0:
	ldy     #$03
        lda     (sp),y
        tay
        lda     _mask_bits,y
	and	$D010
hi_bit_ok:
	sta	$D010

	tya
	asl	a
	tax

        ldy     #$01
	lda     (sp),y
	sta	$D000,x

        ldy     #$00
	lda     (sp),y
	sta	$D001,x

	jmp     incsp4

.endproc

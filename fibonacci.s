 .org $8000
reset:
 ldx #$01 ; x=1
 stx $00 	; stores x at 0x0000
 sec 			; clear carry
 ldy #$07 ; calculate 7th fibonacci number 0x0D
 tya			; transfer Y->A
 sbc #$03 
 tay			; transfer A->Y

 clc
 lda #$02	; a=2
 sta $01	; store a at 0x0001
 
loop:
 ldx $01
 adc $00
 sta $01
 stx $00
 dey
 bne loop
 jmp reset

 .org $FFFA
 .word $9000
 .word reset
 rti

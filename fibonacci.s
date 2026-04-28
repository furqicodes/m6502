 .org $8000
start:
 ldx #$01 ; x=1
 stx $00 	; stores x at 0x0000
 sec 			; clear carry
 ldy #$0D ; calculate 13th fibonacci number 0xE9
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
 
compare:
 txa	; Copy result into accumulator for comparison
 cmp #$E9 ; Compare with expected result
 beq irq ; Jump to irq_vector if correct result
 brk ; sets the B flag in the stack if wrong result

nmi:
irq:
 jmp irq

 .org $FFFA
 .word nmi
 .word start
 .word irq

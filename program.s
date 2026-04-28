 .org $8000
 
reset:
 lda #$ff
 sta $e0

loop:
 lda $e0
 sbc #$01
 sta $e0
 jmp loop

 .org $fffc
 .word reset
 .word $0000

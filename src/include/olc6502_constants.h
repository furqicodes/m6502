#pragma once

// 6502 CPU Constants and Types
#define RESET_VECTOR 0xFFFC
#define DEFAULT_STACK_POINTER 0xFD
#define STACK_BASE 0x0100

// Legal opcodes for the 6502

//LDA
#define INS_LDA_IM 0xA9
#define INS_LDA_ZP 0xA5
#define INS_LDA_ZPX 0xB5
#define INS_LDA_ABS 0xAD
#define INS_LDA_ABSX 0xBD
#define INS_LDA_ABSY 0xB9
#define INS_LDA_INDX 0xA1
#define INS_LDA_INDY 0xB1
//LDX
#define INS_LDX_IM 0xA2
#define INS_LDX_ZP 0xA6
#define INS_LDX_ZPY 0xB6
#define INS_LDX_ABS 0xAE
#define INS_LDX_ABSY 0xBE
//LDY
#define INS_LDY_IM 0xA0
#define INS_LDY_ZP 0xA4
#define INS_LDY_ZPX 0xB4
#define INS_LDY_ABS 0xAC
#define INS_LDY_ABSX 0xBC
//STA
#define INS_STA_ZP 0x85
#define INS_STA_ZPX 0x95
#define INS_STA_ABS 0x8D
#define INS_STA_ABSX 0x9D
#define INS_STA_ABSY 0x99
#define INS_STA_INDX 0x81
#define INS_STA_INDY 0x91
//STX
#define INS_STX_ZP 0x86
#define INS_STX_ZPY 0x96
#define INS_STX_ABS 0x8E
//STY
#define INS_STY_ZP 0x84
#define INS_STY_ZPX 0x94
#define INS_STY_ABS 0x8C

#define INS_TSX 0xBA
#define INS_TXS 0x9A
#define INS_PHA 0x48
#define INS_PLA 0x68
#define INS_PHP 0x08
#define INS_PLP 0x28

#define INS_JMP_ABS 0x4C
#define INS_JMP_IND 0x6C
#define INS_JSR 0x20
#define INS_RTS 0x60

//Logical Ops

//AND
#define INS_AND_IM 0x29
#define INS_AND_ZP 0x25
#define INS_AND_ZPX 0x35
#define INS_AND_ABS 0x2D
#define INS_AND_ABSX 0x3D
#define INS_AND_ABSY 0x39
#define INS_AND_INDX 0x21
#define INS_AND_INDY 0x31

//OR
#define INS_ORA_IM 0x09
#define INS_ORA_ZP 0x05
#define INS_ORA_ZPX 0x15
#define INS_ORA_ABS 0x0D
#define INS_ORA_ABSX 0x1D
#define INS_ORA_ABSY 0x19
#define INS_ORA_INDX 0x01
#define INS_ORA_INDY 0x11

//EOR
#define INS_EOR_IM 0x49
#define INS_EOR_ZP  0x45
#define INS_EOR_ZPX 0x55
#define INS_EOR_ABS 0x4D
#define INS_EOR_ABSX 0x5D
#define INS_EOR_ABSY 0x59
#define INS_EOR_INDX 0x41
#define INS_EOR_INDY 0x51

//BIT
#define INS_BIT_ZP 0x24
#define INS_BIT_ABS 0x2C

//Transfer Registers
#define INS_TAX 0xAA
#define INS_TAY 0xA8
#define INS_TXA 0x8A
#define INS_TYA 0x98

//Increments Decrements
#define INS_INX 0xE8
#define INS_INY 0xC8
#define INS_DEY 0x88
#define INS_DEX 0xCA
#define INS_DEC_ZP 0xC6
#define INS_DEC_ZPX 0xD6
#define INS_DEC_ABS 0xCE
#define INS_DEC_ABSX 0xDE
#define INS_INC_ZP 0xE6
#define INS_INC_ZPX 0xF6
#define INS_INC_ABS 0xEE
#define INS_INC_ABSX 0xFE

//branches
#define INS_BEQ 0xF0
#define INS_BNE 0xD0
#define INS_BCS 0xB0
#define INS_BCC 0x90
#define INS_BMI 0x30
#define INS_BPL 0x10
#define INS_BVC 0x50
#define INS_BVS 0x70

//status flag changes
#define INS_CLC 0x18
#define INS_SEC 0x38
#define INS_CLD 0xD8
#define INS_SED 0xF8
#define INS_CLI 0x58
#define INS_SEI 0x78
#define INS_CLV 0xB8

//Arithmetic
#define INS_ADC 0x69
#define INS_ADC_ZP 0x65
#define INS_ADC_ZPX 0x75
#define INS_ADC_ABS 0x6D
#define INS_ADC_ABSX 0x7D
#define INS_ADC_ABSY 0x79
#define INS_ADC_INDX 0x61
#define INS_ADC_INDY 0x71

#define INS_SBC 0xE9
#define INS_SBC_ABS 0xED
#define INS_SBC_ZP 0xE5
#define INS_SBC_ZPX 0xF5
#define INS_SBC_ABSX 0xFD
#define INS_SBC_ABSY 0xF9
#define INS_SBC_INDX 0xE1
#define INS_SBC_INDY 0xF1

// Register Comparison
#define INS_CMP 0xC9
#define INS_CMP_ZP 0xC5
#define INS_CMP_ZPX 0xD5
#define INS_CMP_ABS 0xCD
#define INS_CMP_ABSX 0xDD
#define INS_CMP_ABSY 0xD9
#define INS_CMP_INDX 0xC1
#define INS_CMP_INDY 0xD1

#define INS_CPX 0xE0
#define INS_CPY 0xC0
#define INS_CPX_ZP 0xE4
#define INS_CPY_ZP 0xC4
#define INS_CPX_ABS 0xEC
#define INS_CPY_ABS 0xCC

// shifts
#define INS_ASL 0x0A
#define INS_ASL_ZP 0x06
#define INS_ASL_ZPX 0x16
#define INS_ASL_ABS 0x0E
#define INS_ASL_ABSX 0x1E

#define INS_LSR 0x4A
#define INS_LSR_ZP 0x46
#define INS_LSR_ZPX 0x56
#define INS_LSR_ABS 0x4E
#define INS_LSR_ABSX 0x5E

#define INS_ROL 0x2A
#define INS_ROL_ZP 0x26
#define INS_ROL_ZPX 0x36
#define INS_ROL_ABS 0x2E
#define INS_ROL_ABSX 0x3E

#define INS_ROR 0x6A
#define INS_ROR_ZP 0x66
#define INS_ROR_ZPX 0x76
#define INS_ROR_ABS 0x6E
#define INS_ROR_ABSX 0x7E

//misc
#define INS_NOP 0xEA
#define INS_BRK 0x00
#define INS_RTI 0x40

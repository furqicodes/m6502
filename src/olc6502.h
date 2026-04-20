#pragma once

#include <stdint.h>

#include "memory.h"

enum ProcessorStatusFlags {
    CARRY_FLAG = 1 << 0,
    ZERO_FLAG = 1 << 1,
    INTERRUPT_DISABLE_FLAG = 1 << 2,
    DECIMAL_MODE_FLAG = 1 << 3,         // Note: The 6502 does not support decimal mode in all variants, but we include it for completeness.
    BREAK_COMMAND_FLAG = 1 << 4,
    UNUSED_FLAG = 1 << 5,
    OVERFLOW_FLAG = 1 << 6,
    NEGATIVE_FLAG = 1 << 7
};

typedef struct {
    uint8_t A;      // Accumulator
    uint8_t X;      // X Register
    uint8_t Y;      // Y Register
    uint8_t SP;     // Stack Pointer
    uint8_t PS;     // Processor Status Flags
    uint16_t PC;    // Program Counter

} olc6502_t;

/*
 * Initialize the CPU to a known state
 * Also known as the "Power-Up" state
*/
int olc6502_init(olc6502_t* cpu);

// External event functions. In hardware these represent pins that are asserted
// to produce a change in state.
void olc6502_reset(olc6502_t* cpu);	                    // Reset Interrupt - Forces CPU into known state
// void irq(olc6502_t* cpu, memory_t* mem);		// Interrupt Request - Executes an instruction at a specific location
// void nmi(olc6502_t* cpu, memory_t* mem);		// Non-Maskable Interrupt Request - As above, but cannot be disabled
// void clock(olc6502_t* cpu, memory_t* mem);	    // Perform one clock cycle's worth of update
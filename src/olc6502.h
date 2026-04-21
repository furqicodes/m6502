#pragma once

#include <stdint.h>

#include "memory.h"

union Flags {
    uint8_t value;
    struct {
        uint8_t C : 1; // Carry Flag
        uint8_t Z : 1; // Zero Flag
        uint8_t I : 1; // Interrupt Disable Flag
        uint8_t D : 1; // Decimal Mode Flag
        uint8_t B : 1; // Break Command Flag
        uint8_t U : 1; // Unused Flag (always set to 1)
        uint8_t V : 1; // Overflow Flag
        uint8_t N : 1; // Negative Flag
    };
};

typedef struct {
    uint8_t A;      // Accumulator
    uint8_t X;      // X Register
    uint8_t Y;      // Y Register
    uint8_t SP;     // Stack Pointer
    union Flags PS;     // Processor Status Flags
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
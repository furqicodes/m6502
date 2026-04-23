#pragma once

#include "include/olc6502_constants.h"
#include "ti74ls138.h"

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
    m74ls138_t* CE; // Address decoder
} olc6502_t;

/*
 * Initialize the CPU to a known state
 * Also known as the "Power-Up" state
*/
int olc6502_init(olc6502_t* cpu, m74ls138_t* ce);

/**
 * @brief Execute the CPU for a specified number of cycles
 * 
 * @param cpu Pointer to the CPU state
 * @param cycles The number of cycles to execute
 * @param mem Pointer to the memory 
 *
 * @return The number of cycles consumed
 */
int32_t olc6502_clock(olc6502_t* cpu, int32_t cycles);


/* Helper functions */

/**
* @brief Fetch an operand from memory and consume 1 cycle
*
* @param cpu Pointer to the CPU state
* @param mem Pointer to the memory
* @param cycles Pointer to the remaining cycles, which will be decremented by 1
* 
* @return The byte value fetched from memory at the current PC
*/
uint8_t fetch_operand(olc6502_t* cpu, int32_t* cycles);

/**
 * @brief Get the absolute address for an instruction and consume 2 cycles
 * 
 * @param cpu Pointer to the CPU state
 * @param mem Pointer to the memory
 * @param cycles Pointer to the remaining cycles, which will be decremented by 2
 * 
 * @return The absolute address calculated from the next two bytes in memory
 *
 */
uint16_t get_absolute_address(olc6502_t* cpu, int32_t* cycles);

/**
 * @brief Get the absolute address for an instruction with X index and consume 2 cycles (plus 1 additional cycle if page boundary is crossed)
 * 
 * @param cpu Pointer to the CPU state
 * @param cycles Pointer to the remaining cycles, which will be decremented by 2 (plus 1 additional cycle if page boundary is crossed)
 * 
 * @return The absolute address calculated from the next two bytes in memory, with the offset of X register
 */
uint16_t get_absolute_addressX(olc6502_t* cpu, int32_t* cycles);

/**
 * @brief Get the absolute address for an instruction with Y index and consume 2 cycles (plus 1 additional cycle if page boundary is crossed)
 * 
 * @param cpu Pointer to the CPU state
 * @param cycles Pointer to the remaining cycles, which will be decremented by 2 (plus 1 additional cycle if page boundary is crossed)
 * 
 * @return The absolute address calculated from the next two bytes in memory, with the offset of Y register
 */
uint16_t get_absolute_addressY(olc6502_t* cpu, int32_t* cycles);

/**
 * @brief Get the indirect address for an instruction with X offset and consume 3 cycles
 * 
 * @param cpu Pointer to the CPU state
 * @param cycles Pointer to the remaining cycles, which will be decremented by 3
 * 
 * @return The indirect address calculated by first adding the X register to the next byte in memory to get an absolute address, then reading the 16-bit address from that absolute address location
 */
uint16_t get_indexed_indirectX(olc6502_t* cpu, int32_t* cycles);

uint16_t get_zp_address(olc6502_t* cpu, int32_t* cycles);

/**
 * @brief Push a 16-bit word onto the stack and consume 2 cycles
 * 
 * @param cpu Pointer to the CPU state
 * @param mem Pointer to the memory
 * @param value The 16-bit value to push onto the stack
 * @param cycles Pointer to the remaining cycles, which will be decremented by 2
 * 
 * This function pushes the high byte of the value first, then the low byte, and updates the stack pointer accordingly.
 */
void push_word_to_stack(olc6502_t* cpu, uint16_t value, int32_t* cycles);

/**
 * @brief Pull a 16-bit word from the stack and consume 2 cycles
 * 
 * @param cpu Pointer to the CPU state
 * @param mem Pointer to the memory
 * @param cycles Pointer to the remaining cycles, which will be decremented by 2
 * 
 * @return The 16-bit value pulled from the stack, with the low byte pulled first followed by the high byte
 */
uint16_t pull_word_from_stack(olc6502_t* cpu, int32_t* cycles);

// External event functions. In hardware these represent pins that are asserted
// to produce a change in state.
void olc6502_reset(olc6502_t* cpu);	                    // Reset Interrupt - Forces CPU into known state
// void irq(olc6502_t* cpu, memory_t* mem);		// Interrupt Request - Executes an instruction at a specific location
// void nmi(olc6502_t* cpu, memory_t* mem);		// Non-Maskable Interrupt Request - As above, but cannot be disabled
// void clock(olc6502_t* cpu, memory_t* mem);	    // Perform one clock cycle's worth of update
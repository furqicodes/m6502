#include "olc6502.h"
#include "include/olc6502_constants.h"

int olc6502_init(olc6502_t* cpu) {
    cpu->A = 0;
    cpu->X = 0;
    cpu->Y = 0;
    cpu->PC = RESET_VECTOR;
    cpu->SP = DEFAULT_STACK_POINTER;
    cpu->PS.value = 0x04; // Clear all flags except the Interrupt Disable Flag
    return 0;
}

void olc6502_reset(olc6502_t* cpu) {
    cpu->PC = RESET_VECTOR;
    cpu->SP = DEFAULT_STACK_POINTER;
    cpu->PS.I = 1; // Set Interrupt Disable Flag
    cpu->PS.D = 0; // Clear Decimal Mode Flag
}

int32_t olc6502_clock(olc6502_t* cpu, int32_t cycles, memory_t* mem) {
    uint32_t requested_cycles = cycles;
    while (cycles > 0) {
    return requested_cycles - cycles;
}

uint16_t get_absolute_address(olc6502_t* cpu, memory_t* mem, int32_t* cycles) {
    uint16_t low = memory_get(mem, cpu->PC++);
    uint16_t high = memory_get(mem, cpu->PC++);
    *cycles -= 2;
    return (high << 8) | low;
}

uint8_t fetch_operand(olc6502_t* cpu, memory_t* mem, int32_t* cycles) {
    *cycles -= 1;
    return memory_get(mem, cpu->PC++);
}

void push_word_to_stack(olc6502_t* cpu, memory_t* mem, uint16_t value, int32_t* cycles) {
    memory_set(mem, STACK_BASE + cpu->SP, value >> 8); // Push high byte
    cpu->SP--;
    memory_set(mem, STACK_BASE + cpu->SP, value & 0xFF); // Push low byte
    cpu->SP--;
    *cycles -= 2;
}

uint16_t pull_word_from_stack(olc6502_t* cpu, memory_t* mem, int32_t* cycles) {
    cpu->SP++;
    uint16_t low = memory_get(mem, STACK_BASE + cpu->SP); // Pull low byte
    cpu->SP++;
    uint16_t high = memory_get(mem, STACK_BASE + cpu->SP); // Pull high byte
    *cycles -= 2;
    return (high << 8) | low;
}
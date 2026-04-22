#include "olc6502.h"
#include <stdbool.h>
// REMOVEME
#include <stdio.h>

int olc6502_init(olc6502_t* cpu, m74ls138_t* ce) {
    cpu->A = 0;
    cpu->X = 0;
    cpu->Y = 0;
    cpu->PC = RESET_VECTOR;
    cpu->SP = DEFAULT_STACK_POINTER;
    cpu->PS.value = 0x04; // Clear all flags except the Interrupt Disable Flag
    cpu->CE = ce;
    return 0;
}

void olc6502_reset(olc6502_t* cpu) {
    cpu->PC = RESET_VECTOR;
    cpu->SP = DEFAULT_STACK_POINTER;
    cpu->PS.I = 1; // Set Interrupt Disable Flag
    cpu->PS.D = 0; // Clear Decimal Mode Flag
}

bool interrupt_flag_should_be_set = false;
uint8_t interrupt_disable_flag_next;
int32_t olc6502_clock(olc6502_t* cpu, int32_t cycles) {
    uint32_t requested_cycles = cycles;
    while (cycles > 0) {
        uint8_t opcode = fetch_operand(cpu, &cycles);

        // Delayed flag setting logic for SEI/CLI: the effect of changing the Interrupt Disable Flag is delayed one instruction because the flag is changed after IRQ is polled, allowing an IRQ to be serviced between this and the next instruction if the flag was previously 0. This means that if SEI is executed, interrupts will still be serviced until the next instruction is executed, and if CLI is executed, interrupts will not be serviced until the next instruction is executed.
        if (interrupt_flag_should_be_set) {
            cpu->PS.I = interrupt_disable_flag_next;
            interrupt_disable_flag_next = false;
        }
        
        switch (opcode)
        {
        // Access type instructions
        case INS_LDA_IM:
            cpu->A = fetch_operand(cpu, &cycles);
            // Set Zero Flag and Negative Flag based on the value loaded into A
            cpu->PS.Z = (cpu->A == 0);
            cpu->PS.N = (cpu->A & 0x80) != 0;
            break;
        case INS_LDX_IM:
            cpu->X = fetch_operand(cpu, &cycles);
            cpu->PS.Z = (cpu->X == 0);
            cpu->PS.N = (cpu->X & 0x80) != 0;
            break;
        case INS_LDY_IM:
            cpu->Y = fetch_operand(cpu, &cycles);
            cpu->PS.Z = (cpu->Y == 0);
            cpu->PS.N = (cpu->Y & 0x80) != 0;
            break;
        // Flags type instructions
        case INS_CLC:
            cpu->PS.C = 0;
            cycles--;
            break;
        case INS_SEC:
            cpu->PS.C = 1;
            cycles--;
            break;
        case INS_CLD:
            cpu->PS.D = 0;
            cycles--;
            break;
        case INS_SED:
            cpu->PS.D = 1;
            cycles--;
            break;
        case INS_CLI:
            interrupt_disable_flag_next = 0;
            interrupt_flag_should_be_set = true;
            cycles--;
            break;
        case INS_SEI:
            interrupt_disable_flag_next = 1;
            interrupt_flag_should_be_set = true;
            cycles--;
            break;
        case INS_CLV:
            cpu->PS.V = 0;
            cycles--;
            break;
        // Jump type instructions
        case INS_JMP_ABS:
            cpu->PC = get_absolute_address(cpu, &cycles);
            break;
        case INS_JMP_IND:
            printf("NOT IMPLEMENTED: JMP (indirect)\n");
            break;
        case INS_JSR:
            uint16_t jump_address = get_absolute_address(cpu, &cycles);
            push_word_to_stack(cpu, cpu->PC - 1, &cycles); // Push return
            cpu->PC = jump_address;
            cycles -= 1; // FIXME: integrate the cycle logic into the functions
            break;
        case INS_RTS:
            cpu->PC = pull_word_from_stack(cpu, &cycles) + 1; // Pull return address and add 1 to get the correct return point
            cycles -= 3;
            break;
        case INS_BRK:
            printf("NOT IMPLEMENTED: BRK\n");
            break;
        case INS_RTI:
            printf("NOT IMPLEMENTED: RTI\n");
            break;
        // ...
        case INS_NOP:
            printf("NOP executed at PC: 0x%04X\n", cpu->PC - 1);
            cycles--;
            break;
        default:
            break;
        }
    }
    return requested_cycles - cycles;
}

uint16_t get_absolute_address(olc6502_t* cpu, int32_t* cycles) {
    uint16_t abs_address = bus_read_word(cpu->CE, cpu->PC);
    cpu->PC += 2;
    *cycles -= 2;
    return abs_address;
}

uint16_t get_zp_address(olc6502_t* cpu, int32_t* cycles) {
    uint8_t zp_address = bus_read_byte(cpu->CE, cpu->PC++);
    *cycles -= 1;
    return zp_address;
}

uint8_t fetch_operand(olc6502_t* cpu, int32_t* cycles) {
    *cycles -= 1;
    return bus_read_byte(cpu->CE, cpu->PC++);
}

void push_word_to_stack(olc6502_t* cpu, uint16_t value, int32_t* cycles) {
    bus_write_byte(cpu->CE, STACK_BASE + cpu->SP, value >> 8); // Push high byte
    cpu->SP--;
    bus_write_byte(cpu->CE, STACK_BASE + cpu->SP, value & 0xFF); // Push low byte
    cpu->SP--;
    *cycles -= 2;
}

uint16_t pull_word_from_stack(olc6502_t* cpu, int32_t* cycles) {
    cpu->SP++;
    uint16_t low = bus_read_byte(cpu->CE, STACK_BASE + cpu->SP); // Pull low byte
    cpu->SP++;
    uint16_t high = bus_read_byte(cpu->CE, STACK_BASE + cpu->SP); // Pull high byte
    *cycles -= 2;
    return (high << 8) | low;
}
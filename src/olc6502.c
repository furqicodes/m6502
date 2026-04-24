#include "olc6502.h"
#include <stdbool.h>
// REMOVEME
#include <stdio.h>

int olc6502_init(olc6502_t* cpu, m74ls138_t* ce) {
    cpu->A = 0;
    cpu->X = 0;
    cpu->Y = 0;
    cpu->PC = RESET_VECTOR;
    cpu->SP = STACK_BASE - 3;
    cpu->PS.value = 0x04; // Clear all flags except the Interrupt Disable Flag
    cpu->CE = ce;
    return 0;
}

void olc6502_reset(olc6502_t* cpu, int32_t* cycles) {
    cpu->PC = RESET_VECTOR;
    cpu->SP = STACK_BASE - 3;
    cpu->PS.I = 1; // Set Interrupt Disable Flag
    cpu->PS.D = 0; // Clear Decimal Mode Flag
    *cycles -= 7;
    // When the 6502 is reset, the PC is set to the RESET_VECTOR address, next 2 bytes are read from
    // that address to set the program start point
    uint16_t prg_start = bus_read_word(cpu->CE, RESET_VECTOR);
    cpu->PC = prg_start;
}

inline void update_flags_from_register(olc6502_t* cpu, uint8_t reg) {
    // Set Zero Flag and Negative Flag based on the value in the register
    cpu->PS.Z = (reg == 0);
    cpu->PS.N = (reg & 0x80) != 0;
}

/* The 6502 has one 8-bit ALU and one 16-bit upcounter (for PC). To calculate a,x or a,y addressing in 
 * an instruction other than sta, stx, or sty, it uses the 8-bit ALU to first calculate the low byte while 
 * it fetches the high byte. If there's a carry out, it goes "oops", applies the carry using the ALU, and 
 * repeats the read at the correct address. Store instructions always have this "oops" cycle: the CPU first 
 * reads from the partially added address and then writes to the correct address. The same thing happens 
 * on (d),y indirect addressing.
 */
bool oops_cycle = false;
inline bool page_boundary_crossed(uint16_t addr1, uint16_t addr2) {
    oops_cycle = true;
    return (addr1 & 0xFF00) != (addr2 & 0xFF00);
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
            update_flags_from_register(cpu, cpu->A);
            break;
        case INS_LDA_ZP:
            cpu->A = bus_read_byte(cpu->CE, get_zp_address(cpu, &cycles));
            update_flags_from_register(cpu, cpu->A);
            break;
        case INS_LDA_ZPX:
            uint8_t zp_address = (get_zp_address(cpu, &cycles) + cpu->X) & 0xFF; // Wrap around zero page
            cpu->A = bus_read_byte(cpu->CE, zp_address);
            cycles -= 2;
            update_flags_from_register(cpu, cpu->A);
            break;
        case INS_LDA_ABS:
            cpu->A = bus_read_byte(cpu->CE, get_absolute_address(cpu, &cycles));
            cycles -= 1; // Additional cycle for absolute addressing mode
            update_flags_from_register(cpu, cpu->A);
            break;
        case INS_LDA_ABSX:
            uint16_t abs_addressX = get_absolute_addressX(cpu, &cycles);
            cpu->A = bus_read_byte(cpu->CE, abs_addressX);
            cycles -= oops_cycle ? 2 : 1;
            oops_cycle = false;
            update_flags_from_register(cpu, cpu->A);
            break;
        case INS_LDA_ABSY:
            uint16_t abs_addressY = get_absolute_addressY(cpu, &cycles);
            cpu->A = bus_read_byte(cpu->CE, abs_addressY);
            cycles -= oops_cycle ? 2 : 1;
            oops_cycle = false;
            update_flags_from_register(cpu, cpu->A);
            break;
        case INS_LDA_INDX:
            cpu->A = bus_read_byte(cpu->CE, get_indexed_indirectX(cpu, &cycles));
            cycles -= 2;
            update_flags_from_register(cpu, cpu->A);
            break;
        case INS_LDA_INDY:
            cpu->A = bus_read_byte(cpu->CE, get_indexed_indirectY(cpu, &cycles));
            cycles -= oops_cycle ? 2 : 1;
            oops_cycle = false;
            update_flags_from_register(cpu, cpu->A);
            break;
        case INS_STA_ZP:
            bus_write_byte(cpu->CE, get_zp_address(cpu, &cycles), cpu->A);
            cycles -= 1;
            break;
        case INS_STA_ZPX:
            uint8_t zpx_address = (get_zp_address(cpu, &cycles) + cpu->X) & 0xFF;
            bus_write_byte(cpu->CE, zpx_address, cpu->A);
            cycles -= 2;
            break;
        case INS_STA_ABS:
            uint16_t abs_address = get_absolute_address(cpu, &cycles);
            bus_write_byte(cpu->CE, abs_address, cpu->A);
            cycles -= 1;
            break;
        case INS_STA_ABSX:
            uint16_t sta_absx_address = get_absolute_addressX(cpu, &cycles);
            bus_write_byte(cpu->CE, sta_absx_address, cpu->A);
            cycles -= 2;
            oops_cycle = false;
            break;
        case INS_STA_ABSY:
            uint16_t sta_absy_address = get_absolute_addressY(cpu, &cycles);
            bus_write_byte(cpu->CE, sta_absy_address, cpu->A);
            cycles -= 2;
            oops_cycle = false;
            break;
        case INS_STA_INDX:
            uint16_t indexed_indirectX_address = get_indexed_indirectX(cpu, &cycles);
            bus_write_byte(cpu->CE, indexed_indirectX_address, cpu->A);
            cycles -= 2;
            break;
        case INS_STA_INDY:
            uint16_t indexed_indirectY_address = get_indexed_indirectY(cpu, &cycles);
            bus_write_byte(cpu->CE, indexed_indirectY_address, cpu->A);
            cycles -= 2;
            oops_cycle = false;
            break;
        case INS_LDX_IM:
            cpu->X = fetch_operand(cpu, &cycles);
            update_flags_from_register(cpu, cpu->X);
            break;
        case INS_LDX_ZP:
            cpu->X = bus_read_byte(cpu->CE, get_zp_address(cpu, &cycles));
            cycles -= 1;
            update_flags_from_register(cpu, cpu->X);
            break;
        case INS_LDX_ZPY:
            uint8_t zpy_address = (get_zp_address(cpu, &cycles) + cpu->Y) & 0xFF;
            cpu->X = bus_read_byte(cpu->CE, zpy_address);
            cycles -= 2;
            update_flags_from_register(cpu, cpu->X);
            break;
        case INS_LDX_ABS:
            cpu->X = bus_read_byte(cpu->CE, get_absolute_address(cpu, &cycles));
            cycles -= 1;
            update_flags_from_register(cpu, cpu->X);
            break;
        case INS_LDX_ABSY:
            uint16_t ldx_absy_address = get_absolute_addressY(cpu, &cycles);
            cpu->X = bus_read_byte(cpu->CE, ldx_absy_address);
            cycles -= oops_cycle ? 2 : 1;
            oops_cycle = false;
            update_flags_from_register(cpu, cpu->X);
            break;
        case INS_STX_ZP:
            bus_write_byte(cpu->CE, get_zp_address(cpu, &cycles), cpu->X);
            cycles -= 1;
            break;
        case INS_STX_ZPY:
            uint8_t stx_zpy_address = (get_zp_address(cpu, &cycles) + cpu->Y) & 0xFF;
            bus_write_byte(cpu->CE, stx_zpy_address, cpu->X);
            cycles -= 2;
            break;
        case INS_STX_ABS:
            uint16_t stx_abs_address = get_absolute_address(cpu, &cycles);
            bus_write_byte(cpu->CE, stx_abs_address, cpu->X);
            cycles -= 1;
            break;
        case INS_LDY_IM:
            cpu->Y = fetch_operand(cpu, &cycles);
            update_flags_from_register(cpu, cpu->Y);
            break;
        case INS_LDY_ZP:
            cpu->Y = bus_read_byte(cpu->CE, get_zp_address(cpu, &cycles));
            cycles -= 1;
            update_flags_from_register(cpu, cpu->Y);
            break;
        case INS_LDY_ZPX:
            uint8_t ldy_zpx_address = (get_zp_address(cpu, &cycles) + cpu->X) & 0xFF;
            cpu->Y = bus_read_byte(cpu->CE, ldy_zpx_address);
            cycles -= 2;
            update_flags_from_register(cpu, cpu->Y);
            break;
        case INS_LDY_ABS:
            cpu->Y = bus_read_byte(cpu->CE, get_absolute_address(cpu, &cycles));
            cycles -= 1;
            update_flags_from_register(cpu, cpu->Y);
            break;
        case INS_LDY_ABSX:
            uint16_t ldy_absx_address = get_absolute_addressX(cpu, &cycles);
            cpu->Y = bus_read_byte(cpu->CE, ldy_absx_address);
            cycles -= oops_cycle ? 2 : 1;
            oops_cycle = false;
            update_flags_from_register(cpu, cpu->Y);
            break;
        case INS_STY_ZP:
            bus_write_byte(cpu->CE, get_zp_address(cpu, &cycles), cpu->Y);
            cycles -= 1;
            break;
        case INS_STY_ZPX:
            uint8_t sty_zpx_address = (get_zp_address(cpu, &cycles) + cpu->X) & 0xFF;
            bus_write_byte(cpu->CE, sty_zpx_address, cpu->Y);
            cycles -= 2;
            break;
        case INS_STY_ABS:
            uint16_t sty_abs_address = get_absolute_address(cpu, &cycles);
            bus_write_byte(cpu->CE, sty_abs_address, cpu->Y);
            cycles -= 1;
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

uint16_t get_absolute_addressX(olc6502_t* cpu, int32_t* cycles) {
    uint16_t abs_address = bus_read_word(cpu->CE, cpu->PC);
    uint16_t final_address = abs_address + cpu->X;
    cpu->PC += 2;
    *cycles -= 2;
    oops_cycle = page_boundary_crossed(abs_address, final_address);
    return final_address;
}

uint16_t get_indexed_indirectX(olc6502_t* cpu, int32_t* cycles) {
    // val = PEEK(PEEK((arg + X) % 256) + PEEK((arg + X + 1) % 256) * 256)
    uint16_t arg = get_zp_address(cpu, cycles);
    uint16_t indirect_address = bus_read_word(cpu->CE, arg + cpu->X);
    return indirect_address;
}

uint16_t get_indexed_indirectY(olc6502_t* cpu, int32_t* cycles) {
    // val = PEEK(PEEK(arg) + PEEK((arg + 1) % 256) * 256 + Y)
    uint16_t arg = get_zp_address(cpu, cycles);
    uint16_t base_address = bus_read_word(cpu->CE, arg);
    uint16_t final_address = base_address + cpu->Y;
    *cycles -= 2;
    oops_cycle = page_boundary_crossed(base_address, final_address);
    return final_address;
}

uint16_t get_absolute_addressY(olc6502_t* cpu, int32_t* cycles) {
    uint16_t abs_address = bus_read_word(cpu->CE, cpu->PC);
    uint16_t final_address = abs_address + cpu->Y;
    cpu->PC += 2;
    *cycles -= 2;
    oops_cycle = page_boundary_crossed(abs_address, final_address);
    return final_address;
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
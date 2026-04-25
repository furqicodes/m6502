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
    cpu->I_nxt = 1;
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
    cpu->I_nxt = 1;
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

static inline void branch_if(olc6502_t* cpu, int32_t* cycles, bool condition) {
    if (!condition) {
        fetch_operand(cpu, cycles);
        return;
    }
    uint16_t old_PC = cpu->PC;
    int8_t offset = (int8_t)fetch_operand(cpu, cycles);
    cpu->PC += offset;
    *cycles -= page_boundary_crossed(old_PC - 1, cpu->PC) ? 2 : 1;
}

int32_t olc6502_clock(olc6502_t* cpu, int32_t cycles) {
    uint32_t requested_cycles = cycles;
    while (cycles > 0) {
        uint8_t opcode = fetch_operand(cpu, &cycles);

        cpu->PS.I = cpu->I_nxt;
        
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
        // Transfer type instructions
        case INS_TAX:
            cpu->X = cpu->A;
            update_flags_from_register(cpu, cpu->X);
            cycles -= 1;
            break;
        case INS_TXA:
            cpu->A = cpu->X;
            update_flags_from_register(cpu, cpu->A);
            cycles -= 1;
            break;
        case INS_TAY:
            cpu->Y = cpu->A;
            update_flags_from_register(cpu, cpu->Y);
            cycles -= 1;
            break;
        case INS_TYA:
            cpu->A = cpu->Y;
            update_flags_from_register(cpu, cpu->A);
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
            cpu->I_nxt = 0;
            cycles--;
            break;
        case INS_SEI:
            cpu->I_nxt = 1;
            cycles--;
            break;
        case INS_CLV:
            cpu->PS.V = 0;
            cycles--;
            break;
        // Compare type instructions
        case INS_CMP_IM:
            uint8_t cmp_value = fetch_operand(cpu, &cycles);
            cpu->PS.C = (cpu->A >= cmp_value);
            update_flags_from_register(cpu, cpu->A - cmp_value);
            break;
        case INS_CMP_ZP:
            uint8_t cmp_zp_value = bus_read_byte(cpu->CE, get_zp_address(cpu, &cycles));
            cycles -= 1;
            cpu->PS.C = (cpu->A >= cmp_zp_value);
            update_flags_from_register(cpu, cpu->A - cmp_zp_value);
            break;
        case INS_CMP_ZPX:
            uint8_t cmp_zpx_address = (get_zp_address(cpu, &cycles) + cpu->X) & 0xFF;
            uint8_t cmp_zpx_value = bus_read_byte(cpu->CE, cmp_zpx_address);
            cycles -= 2;
            cpu->PS.C = (cpu->A >= cmp_zpx_value);
            update_flags_from_register(cpu, cpu->A - cmp_zpx_value);
            break;
        case INS_CMP_ABS:
            uint16_t cmp_abs_address = get_absolute_address(cpu, &cycles);
            uint8_t cmp_abs_value = bus_read_byte(cpu->CE, cmp_abs_address);
            cycles -= 1;
            cpu->PS.C = (cpu->A >= cmp_abs_value);
            update_flags_from_register(cpu, cpu->A - cmp_abs_value);
            break;
        case INS_CMP_ABSX:
            uint16_t cmp_absx_address = get_absolute_addressX(cpu, &cycles);
            uint8_t cmp_absx_value = bus_read_byte(cpu->CE, cmp_absx_address);
            cycles -= oops_cycle ? 2 : 1;
            oops_cycle = false;
            cpu->PS.C = (cpu->A >= cmp_absx_value);
            update_flags_from_register(cpu, cpu->A - cmp_absx_value);
            break;
        case INS_CMP_ABSY:
            uint16_t cmp_abyx_address = get_absolute_addressY(cpu, &cycles);
            uint8_t cmp_abyx_value = bus_read_byte(cpu->CE, cmp_abyx_address);
            cycles -= oops_cycle ? 2 : 1;
            oops_cycle = false;
            cpu->PS.C = (cpu->A >= cmp_abyx_value);
            update_flags_from_register(cpu, cpu->A - cmp_abyx_value);
            break;
        case INS_CMP_INDX:
            uint16_t cmp_indexed_indirectX_address = get_indexed_indirectX(cpu, &cycles);
            uint8_t cmp_indexed_indirectX_value = bus_read_byte(cpu->CE, cmp_indexed_indirectX_address);
            cycles -= 2;
            cpu->PS.C = (cpu->A >= cmp_indexed_indirectX_value);
            update_flags_from_register(cpu, cpu->A - cmp_indexed_indirectX_value);
            break;
        case INS_CMP_INDY:
            uint16_t cmp_indexed_indirectY_address = get_indexed_indirectY(cpu, &cycles);
            uint8_t cmp_indexed_indirectY_value = bus_read_byte(cpu->CE, cmp_indexed_indirectY_address);
            cycles -= oops_cycle ? 2 : 1;
            oops_cycle = false;
            cpu->PS.C = (cpu->A >= cmp_indexed_indirectY_value);
            update_flags_from_register(cpu, cpu->A - cmp_indexed_indirectY_value);
            break;
        case INS_CPX_IM:
            uint8_t cpx_im_value = fetch_operand(cpu, &cycles);
            cpu->PS.C = (cpu->X >= cpx_im_value);
            update_flags_from_register(cpu, cpu->X - cpx_im_value);
            break;
        case INS_CPX_ZP:
            uint8_t cpx_zp_value = bus_read_byte(cpu->CE, get_zp_address(cpu, &cycles));
            cycles -= 1;
            cpu->PS.C = (cpu->X >= cpx_zp_value);
            update_flags_from_register(cpu, cpu->X - cpx_zp_value);
            break;
        case INS_CPX_ABS:
            uint16_t cpx_abs_address = get_absolute_address(cpu, &cycles);
            uint8_t cpx_abs_value = bus_read_byte(cpu->CE, cpx_abs_address);
            cycles -= 1;
            cpu->PS.C = (cpu->X >= cpx_abs_value);
            update_flags_from_register(cpu, cpu->X - cpx_abs_value);
            break;
        case INS_CPY_IM:
            uint8_t cpy_im_value = fetch_operand(cpu, &cycles);
            cpu->PS.C = (cpu->Y >= cpy_im_value);
            update_flags_from_register(cpu, cpu->Y - cpy_im_value);
            break;
        case INS_CPY_ZP:
            uint8_t cpy_zp_value = bus_read_byte(cpu->CE, get_zp_address(cpu, &cycles));
            cycles -= 1;
            cpu->PS.C = (cpu->Y >= cpy_zp_value);
            update_flags_from_register(cpu, cpu->Y - cpy_zp_value);
            break;
        case INS_CPY_ABS:
            uint16_t cpy_abs_address = get_absolute_address(cpu, &cycles);
            uint8_t cpy_abs_value = bus_read_byte(cpu->CE, cpy_abs_address);
            cycles -= 1;
            cpu->PS.C = (cpu->Y >= cpy_abs_value);
            update_flags_from_register(cpu, cpu->Y - cpy_abs_value);
            break;
        // Branch type instructions
        case INS_BCC:
            branch_if(cpu, &cycles, !cpu->PS.C);
            break;
        case INS_BCS:
            branch_if(cpu, &cycles, cpu->PS.C);
            break;
        case INS_BEQ:
            branch_if(cpu, &cycles, cpu->PS.Z);
            break;
        case INS_BNE:
            branch_if(cpu, &cycles, !cpu->PS.Z);
            break;
        case INS_BPL:
            branch_if(cpu, &cycles, !cpu->PS.N);
            break;
        case INS_BMI:
            branch_if(cpu, &cycles, cpu->PS.N);
            break;
        case INS_BVC:
            branch_if(cpu, &cycles, !cpu->PS.V);
            break;
        case INS_BVS:
            branch_if(cpu, &cycles, cpu->PS.V);
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
        // Stack type instructions
        case INS_PHA:
            bus_write_byte(cpu->CE, STACK_BASE + cpu->SP, cpu->A);
            cpu->SP--;
            cycles -= 2;
            break;
        case INS_PLA:
            cpu->SP++;
            cpu->A = bus_read_byte(cpu->CE, STACK_BASE + cpu->SP);
            update_flags_from_register(cpu, cpu->A);
            cycles -= 3;
            break;
        case INS_PHP:
            bus_write_byte(cpu->CE, STACK_BASE + cpu->SP, cpu->PS.value | 0x30);
            cpu->SP--;
            cycles -= 2;
            break;
        case INS_PLP:
            cpu->SP++;
            uint8_t prev_state = cpu->PS.value;
            uint8_t new_state = bus_read_byte(cpu->CE, STACK_BASE + cpu->SP) & 0xCF; // Ignoring bits 4 and 5
            if ((new_state & 0x04) ^ (prev_state & 0x04)) {
                // Interrupt Disable from 0->1 or 1->0, set the delayed flag update
                cpu->I_nxt = (new_state & 0x04);
            }
            cpu->PS.value |= new_state & 0xFB;  // Ignore Interrupt disable flag for this cycle, it will be updated at the start of the next instruction
            cycles -= 3;
            break;
        case INS_TXS:
            bus_write_byte(cpu->CE, STACK_BASE + cpu->SP, cpu->X);
            cycles -= 1;
            break;
        case INS_TSX:
            cpu->X = bus_read_byte(cpu->CE, STACK_BASE + cpu->SP);
            update_flags_from_register(cpu, cpu->X);
            cycles -= 1;
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
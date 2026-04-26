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
    // Opcode section fields
    uint8_t aaa, bbb, cc;
    uint32_t requested_cycles = cycles;
    uint16_t addr = 0x0000;
    uint8_t fetched = 0x00;
    uint8_t temp = 0x00;
    while (cycles > 0) {
        uint8_t opcode = fetch_operand(cpu, &cycles);
        aaa = (opcode >> 5) & 0x07;
        bbb = (opcode >> 2) & 0x07;
        cc = opcode & 0x03;
        printf("0x%02X: Opcode 0x%02X (a=%03X, b=%u, c=%u), Cycles left: %d\n",
            cpu->PC - 1, opcode, aaa, bbb, cc, cycles);     // REMOVEME

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
            addr = (get_zp_address(cpu, &cycles) + cpu->X) & 0xFF;
            cpu->A = bus_read_byte(cpu->CE, addr);
            cycles -= 2;
            update_flags_from_register(cpu, cpu->A);
            break;
        case INS_LDA_ABS:
            cpu->A = bus_read_byte(cpu->CE, get_absolute_address(cpu, &cycles));
            cycles -= 1; // Additional cycle for absolute addressing mode
            update_flags_from_register(cpu, cpu->A);
            break;
        case INS_LDA_ABSX:
            addr = get_absolute_addressX(cpu, &cycles);
            cpu->A = bus_read_byte(cpu->CE, addr);
            cycles -= oops_cycle ? 2 : 1;
            oops_cycle = false;
            update_flags_from_register(cpu, cpu->A);
            break;
        case INS_LDA_ABSY:
            addr = get_absolute_addressY(cpu, &cycles);
            cpu->A = bus_read_byte(cpu->CE, addr);
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
            addr = (get_zp_address(cpu, &cycles) + cpu->X) & 0xFF;
            bus_write_byte(cpu->CE, addr, cpu->A);
            cycles -= 2;
            break;
        case INS_STA_ABS:
            addr = get_absolute_address(cpu, &cycles);
            bus_write_byte(cpu->CE, addr, cpu->A);
            cycles -= 1;
            break;
        case INS_STA_ABSX:
            addr = get_absolute_addressX(cpu, &cycles);
            bus_write_byte(cpu->CE, addr, cpu->A);
            cycles -= 2;
            oops_cycle = false;
            break;
        case INS_STA_ABSY:
            addr = get_absolute_addressY(cpu, &cycles);
            bus_write_byte(cpu->CE, addr, cpu->A);
            cycles -= 2;
            oops_cycle = false;
            break;
        case INS_STA_INDX:
            addr = get_indexed_indirectX(cpu, &cycles);
            bus_write_byte(cpu->CE, addr, cpu->A);
            cycles -= 2;
            break;
        case INS_STA_INDY:
            addr = get_indexed_indirectY(cpu, &cycles);
            bus_write_byte(cpu->CE, addr, cpu->A);
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
            addr = (get_zp_address(cpu, &cycles) + cpu->Y) & 0xFF;
            cpu->X = bus_read_byte(cpu->CE, addr);
            cycles -= 2;
            update_flags_from_register(cpu, cpu->X);
            break;
        case INS_LDX_ABS:
            cpu->X = bus_read_byte(cpu->CE, get_absolute_address(cpu, &cycles));
            cycles -= 1;
            update_flags_from_register(cpu, cpu->X);
            break;
        case INS_LDX_ABSY:
            addr = get_absolute_addressY(cpu, &cycles);
            cpu->X = bus_read_byte(cpu->CE, addr);
            cycles -= oops_cycle ? 2 : 1;
            oops_cycle = false;
            update_flags_from_register(cpu, cpu->X);
            break;
        case INS_STX_ZP:
            bus_write_byte(cpu->CE, get_zp_address(cpu, &cycles), cpu->X);
            cycles -= 1;
            break;
        case INS_STX_ZPY:
            addr = (get_zp_address(cpu, &cycles) + cpu->Y) & 0xFF;
            bus_write_byte(cpu->CE, addr, cpu->X);
            cycles -= 2;
            break;
        case INS_STX_ABS:
            addr = get_absolute_address(cpu, &cycles);
            bus_write_byte(cpu->CE, addr, cpu->X);
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
            addr = (get_zp_address(cpu, &cycles) + cpu->X) & 0xFF;
            cpu->Y = bus_read_byte(cpu->CE, addr);
            cycles -= 2;
            update_flags_from_register(cpu, cpu->Y);
            break;
        case INS_LDY_ABS:
            cpu->Y = bus_read_byte(cpu->CE, get_absolute_address(cpu, &cycles));
            cycles -= 1;
            update_flags_from_register(cpu, cpu->Y);
            break;
        case INS_LDY_ABSX:
            addr = get_absolute_addressX(cpu, &cycles);
            cpu->Y = bus_read_byte(cpu->CE, addr);
            cycles -= oops_cycle ? 2 : 1;
            oops_cycle = false;
            update_flags_from_register(cpu, cpu->Y);
            break;
        case INS_STY_ZP:
            bus_write_byte(cpu->CE, get_zp_address(cpu, &cycles), cpu->Y);
            cycles -= 1;
            break;
        case INS_STY_ZPX:
            addr = (get_zp_address(cpu, &cycles) + cpu->X) & 0xFF;
            bus_write_byte(cpu->CE, addr, cpu->Y);
            cycles -= 2;
            break;
        case INS_STY_ABS:
            addr = get_absolute_address(cpu, &cycles);
            bus_write_byte(cpu->CE, addr, cpu->Y);
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
        // Bitwise type instructions
        case INS_AND_IM:
            cpu->A &= fetch_operand(cpu, &cycles);
            update_flags_from_register(cpu, cpu->A);
            break;
        case INS_AND_ZP:
            cpu->A &= bus_read_byte(cpu->CE, get_zp_address(cpu, &cycles));
            cycles -= 1;
            update_flags_from_register(cpu, cpu->A);
            break;
        case INS_AND_ZPX:
            addr = (get_zp_address(cpu, &cycles) + cpu->X) & 0xFF;
            cpu->A &= bus_read_byte(cpu->CE, addr);
            cycles -= 2;
            update_flags_from_register(cpu, cpu->A);
            break;
        case INS_AND_ABS:
            cpu->A &= bus_read_byte(cpu->CE, get_absolute_address(cpu, &cycles));
            cycles -= 1;
            update_flags_from_register(cpu, cpu->A);
            break;
        case INS_AND_ABSX:
            addr = get_absolute_addressX(cpu, &cycles);
            cpu->A &= bus_read_byte(cpu->CE, addr);
            cycles -= oops_cycle ? 2 : 1;
            oops_cycle = false;
            update_flags_from_register(cpu, cpu->A);
            break;
        case INS_AND_ABSY:
            addr = get_absolute_addressY(cpu, &cycles);
            cpu->A &= bus_read_byte(cpu->CE, addr);
            cycles -= oops_cycle ? 2 : 1;
            oops_cycle = false;
            update_flags_from_register(cpu, cpu->A);
            break;
        case INS_AND_INDX:
            cpu->A &= bus_read_byte(cpu->CE, get_indexed_indirectX(cpu, &cycles));
            cycles -= 2;
            update_flags_from_register(cpu, cpu->A);
            break;
        case INS_AND_INDY:
            addr = get_indexed_indirectY(cpu, &cycles);
            cpu->A &= bus_read_byte(cpu->CE, addr);
            cycles -= oops_cycle ? 2 : 1;
            oops_cycle = false;
            update_flags_from_register(cpu, cpu->A);
            break;
        case INS_ORA_IM:
            cpu->A |= fetch_operand(cpu, &cycles);
            update_flags_from_register(cpu, cpu->A);
            break;
        case INS_ORA_ZP:
            cpu->A |= bus_read_byte(cpu->CE, get_zp_address(cpu, &cycles));
            cycles -= 1;
            update_flags_from_register(cpu, cpu->A);
            break;
        case INS_ORA_ZPX:
            addr = (get_zp_address(cpu, &cycles) + cpu->X) & 0xFF;
            cpu->A |= bus_read_byte(cpu->CE, addr);
            cycles -= 2;
            update_flags_from_register(cpu, cpu->A);
            break;
        case INS_ORA_ABS:
            cpu->A |= bus_read_byte(cpu->CE, get_absolute_address(cpu, &cycles));
            cycles -= 1;
            update_flags_from_register(cpu, cpu->A);
            break;
        case INS_ORA_ABSX:
            addr = get_absolute_addressX(cpu, &cycles);
            cpu->A |= bus_read_byte(cpu->CE, addr);
            cycles -= oops_cycle ? 2 : 1;
            oops_cycle = false;
            update_flags_from_register(cpu, cpu->A);
            break;
        case INS_ORA_ABSY:
            addr = get_absolute_addressY(cpu, &cycles);
            cpu->A |= bus_read_byte(cpu->CE, addr);
            cycles -= oops_cycle ? 2 : 1;
            oops_cycle = false;
            update_flags_from_register(cpu, cpu->A);
            break;
        case INS_ORA_INDX:
            cpu->A |= bus_read_byte(cpu->CE, get_indexed_indirectX(cpu, &cycles));
            cycles -= 2;
            update_flags_from_register(cpu, cpu->A);
            break;
        case INS_ORA_INDY:
            addr = get_indexed_indirectY(cpu, &cycles);
            cpu->A |= bus_read_byte(cpu->CE, addr);
            cycles -= oops_cycle ? 2 : 1;
            oops_cycle = false;
            update_flags_from_register(cpu, cpu->A);
            break;
        case INS_EOR_IM:
            cpu->A ^= fetch_operand(cpu, &cycles);
            update_flags_from_register(cpu, cpu->A);
            break;
        case INS_EOR_ZP:
            cpu->A ^= bus_read_byte(cpu->CE, get_zp_address(cpu, &cycles));
            cycles -= 1;
            update_flags_from_register(cpu, cpu->A);
            break;
        case INS_EOR_ZPX:
            addr = (get_zp_address(cpu, &cycles) + cpu->X) & 0xFF;
            cpu->A ^= bus_read_byte(cpu->CE, addr);
            cycles -= 2;
            update_flags_from_register(cpu, cpu->A);
            break;
        case INS_EOR_ABS:
            cpu->A ^= bus_read_byte(cpu->CE, get_absolute_address(cpu, &cycles));
            cycles -= 1;
            update_flags_from_register(cpu, cpu->A);
            break;
        case INS_EOR_ABSX:
            addr = get_absolute_addressX(cpu, &cycles);
            cpu->A ^= bus_read_byte(cpu->CE, addr);
            cycles -= oops_cycle ? 2 : 1;
            oops_cycle = false;
            update_flags_from_register(cpu, cpu->A);
            break;
        case INS_EOR_ABSY:
            addr = get_absolute_addressY(cpu, &cycles);
            cpu->A ^= bus_read_byte(cpu->CE, addr);
            cycles -= oops_cycle ? 2 : 1;
            oops_cycle = false;
            update_flags_from_register(cpu, cpu->A);
            break;
        case INS_EOR_INDX:
            cpu->A ^= bus_read_byte(cpu->CE, get_indexed_indirectX(cpu, &cycles));
            cycles -= 2;
            update_flags_from_register(cpu, cpu->A);
            break;
        case INS_EOR_INDY:
            addr = get_indexed_indirectY(cpu, &cycles);
            cpu->A ^= bus_read_byte(cpu->CE, addr);
            cycles -= oops_cycle ? 2 : 1;
            oops_cycle = false;
            update_flags_from_register(cpu, cpu->A);
            break;
        case INS_BIT_ZP:
            fetched = bus_read_byte(cpu->CE, get_zp_address(cpu, &cycles));
            cycles -= 1;
            cpu->PS.Z = (fetched & cpu->A) == 0;
            cpu->PS.value |= (fetched & 0xC0);
            break;
        case INS_BIT_ABS:
            fetched = bus_read_byte(cpu->CE, get_absolute_address(cpu, &cycles));
            cycles -= 1;
            cpu->PS.Z = (fetched & cpu->A) == 0;
            cpu->PS.value |= (fetched & 0xC0);
            break;
        // Compare type instructions
        case INS_CMP_IM:
            fetched = fetch_operand(cpu, &cycles);
            cpu->PS.C = (cpu->A >= fetched);
            update_flags_from_register(cpu, cpu->A - fetched);
            break;
        case INS_CMP_ZP:
            fetched = bus_read_byte(cpu->CE, get_zp_address(cpu, &cycles));
            cycles -= 1;
            cpu->PS.C = (cpu->A >= fetched);
            update_flags_from_register(cpu, cpu->A - fetched);
            break;
        case INS_CMP_ZPX:
            addr = (get_zp_address(cpu, &cycles) + cpu->X) & 0xFF;
            fetched = bus_read_byte(cpu->CE, addr);
            cycles -= 2;
            cpu->PS.C = (cpu->A >= fetched);
            update_flags_from_register(cpu, cpu->A - fetched);
            break;
        case INS_CMP_ABS:
            addr = get_absolute_address(cpu, &cycles);
            fetched = bus_read_byte(cpu->CE, addr);
            cycles -= 1;
            cpu->PS.C = (cpu->A >= fetched);
            update_flags_from_register(cpu, cpu->A - fetched);
            break;
        case INS_CMP_ABSX:
            addr = get_absolute_addressX(cpu, &cycles);
            fetched = bus_read_byte(cpu->CE, addr);
            cycles -= oops_cycle ? 2 : 1;
            oops_cycle = false;
            cpu->PS.C = (cpu->A >= fetched);
            update_flags_from_register(cpu, cpu->A - fetched);
            break;
        case INS_CMP_ABSY:
            addr = get_absolute_addressY(cpu, &cycles);
            fetched = bus_read_byte(cpu->CE, addr);
            cycles -= oops_cycle ? 2 : 1;
            oops_cycle = false;
            cpu->PS.C = (cpu->A >= fetched);
            update_flags_from_register(cpu, cpu->A - fetched);
            break;
        case INS_CMP_INDX:
            addr = get_indexed_indirectX(cpu, &cycles);
            fetched = bus_read_byte(cpu->CE, addr);
            cycles -= 2;
            cpu->PS.C = (cpu->A >= fetched);
            update_flags_from_register(cpu, cpu->A - fetched);
            break;
        case INS_CMP_INDY:
            addr = get_indexed_indirectY(cpu, &cycles);
            fetched = bus_read_byte(cpu->CE, addr);
            cycles -= oops_cycle ? 2 : 1;
            oops_cycle = false;
            cpu->PS.C = (cpu->A >= fetched);
            update_flags_from_register(cpu, cpu->A - fetched);
            break;
        case INS_CPX_IM:
            fetched = fetch_operand(cpu, &cycles);
            cpu->PS.C = (cpu->X >= fetched);
            update_flags_from_register(cpu, cpu->X - fetched);
            break;
        case INS_CPX_ZP:
            fetched = bus_read_byte(cpu->CE, get_zp_address(cpu, &cycles));
            cycles -= 1;
            cpu->PS.C = (cpu->X >= fetched);
            update_flags_from_register(cpu, cpu->X - fetched);
            break;
        case INS_CPX_ABS:
            addr = get_absolute_address(cpu, &cycles);
            fetched = bus_read_byte(cpu->CE, addr);
            cycles -= 1;
            cpu->PS.C = (cpu->X >= fetched);
            update_flags_from_register(cpu, cpu->X - fetched);
            break;
        case INS_CPY_IM:
            fetched = fetch_operand(cpu, &cycles);
            cpu->PS.C = (cpu->Y >= fetched);
            update_flags_from_register(cpu, cpu->Y - fetched);
            break;
        case INS_CPY_ZP:
            fetched = bus_read_byte(cpu->CE, get_zp_address(cpu, &cycles));
            cycles -= 1;
            cpu->PS.C = (cpu->Y >= fetched);
            update_flags_from_register(cpu, cpu->Y - fetched);
            break;
        case INS_CPY_ABS:
            addr = get_absolute_address(cpu, &cycles);
            fetched = bus_read_byte(cpu->CE, addr);
            cycles -= 1;
            cpu->PS.C = (cpu->Y >= fetched);
            update_flags_from_register(cpu, cpu->Y - fetched);
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
            addr = get_absolute_address(cpu, &cycles);
            push_word_to_stack(cpu, cpu->PC - 1, &cycles); // Push return
            cpu->PC = addr;
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
            fetched = cpu->PS.value;
            temp = bus_read_byte(cpu->CE, STACK_BASE + cpu->SP) & 0xCF; // Ignoring bits 4 and 5
            if ((temp & 0x04) ^ (fetched & 0x04)) {
                // Interrupt Disable from 0->1 or 1->0, set the delayed flag update
                cpu->I_nxt = (temp & 0x04);
            }
            cpu->PS.value |= temp & 0xFB;  // Ignore Interrupt disable flag for this cycle, it will be updated at the start of the next instruction
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

        if (aaa != 4 && bbb == 6 && cc == 0 ) {
            // This is Set/Clear flag instruction
            // CLC : 0b000 SEC : 0b001 CLI : 0b010 SEI : 0b011 CLV : 0b100 CLD : 0b110 SED : 0b111
            // For carry bit: 0b00X, 00 is selector and X is the value
            // For interrupt disable bit: 0b01X, 01 is selector and X is the value
            // For overflow bit: 0b10X, 10 is selector and X is the value
            // For decimal bit: 0b11X, 11 is selector and X is the value
            uint8_t flag_value = aaa & 0x01;
            uint8_t flag_selector = aaa >> 1;
            switch (flag_selector) {
            case 0b00: cpu->PS.C = flag_value; break;
            case 0b01: cpu->I_nxt = flag_value; break;
            case 0b10: cpu->PS.V = flag_value; break;
            case 0b11: cpu->PS.D = flag_value; break;
            }
            cycles--;
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
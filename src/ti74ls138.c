#include "ti74ls138.h"
#include "program.h"

int m74ls138_init(m74ls138_t* dev, memory_t* mem) {
    if (dev == NULL || mem == NULL) {
        return -1;
    }
    dev->CE = CE_CPU_RAM;
    dev->ptr_mem = mem; // Connect the memory bus to the decoder
    return 0;
}

static inline uint16_t chip_select_cpu_ram(uint16_t virtual_address);
static inline uint16_t chip_select_ppu(uint16_t virtual_address);
static inline uint16_t chip_select_apu(uint16_t virtual_address);
static inline uint16_t chip_select_expansion_rom(uint16_t virtual_address);
static inline uint16_t chip_select_sram(uint16_t virtual_address);
static inline uint16_t chip_select_prg_rom(uint16_t virtual_address);

uint16_t decode_address(m74ls138_t* dev, uint16_t emulated_address) {
    // Only check A15-A13 (3 bits) for device selection
    uint8_t address_bits = (emulated_address >> 13) & 0x07; // Extract A15-A13
    dev->CE = (ce_lines_t)address_bits; // Set CE line based on address bits

    uint16_t hw_addr;
    switch(dev->CE) {
        case CE_CPU_RAM:
            hw_addr = chip_select_cpu_ram(emulated_address);
            break;
        case CE_PPU:
            hw_addr = chip_select_ppu(emulated_address) + M6502_RAM_SIZE;
            break;
        case CE_EXPANSION:
            hw_addr = chip_select_expansion_rom(emulated_address) + M6502_RAM_SIZE + PPU_REGISTERS_SIZE;
            break;
        case CE_SRAM:
            hw_addr = chip_select_sram(emulated_address) + M6502_RAM_SIZE + PPU_REGISTERS_SIZE + EXPANSION_ROM_SIZE;
            break;
        case CE_PRG_ROM0:
        case CE_PRG_ROM1:
        case CE_PRG_ROM2:
        case CE_PRG_ROM3:
            hw_addr = chip_select_prg_rom(emulated_address);
            break;
    }

    #ifdef DEBUG
    printf("Decoded address: 0x%04X (emulated) -> 0x%04X (real)\n", emulated_address, hw_addr);
    #endif
    return hw_addr;
}

void bus_write_byte(m74ls138_t* dev, uint16_t address, uint8_t value) {
    uint16_t write_addr = decode_address(dev, address);
    memory_set(dev->ptr_mem, write_addr, value);

}

uint8_t bus_read_byte(m74ls138_t* dev, uint16_t address) {
    uint16_t read_addr = decode_address(dev, address);
    if (address >= 0x8000) {
        return PROGMEM[read_addr];
    }
    return memory_get(dev->ptr_mem, read_addr);
}

void bus_write_word(m74ls138_t* dev, uint16_t address, uint16_t value) {
    uint16_t write_addr = decode_address(dev, address);
    memory_set(dev->ptr_mem, write_addr, value & 0xFF); // Write low byte
    memory_set(dev->ptr_mem, write_addr + 1, (value >> 8) & 0xFF); // Write high byte
}

uint16_t bus_read_word(m74ls138_t* dev, uint16_t address) {
    uint8_t low_byte, high_byte;
    uint16_t read_addr = decode_address(dev, address);
    if (address >= 0x8000) {
        low_byte = PROGMEM[read_addr];
        high_byte = PROGMEM[read_addr + 1];
    } else {
        low_byte = memory_get(dev->ptr_mem, read_addr);
        high_byte = memory_get(dev->ptr_mem, read_addr + 1);
    }
    return (high_byte << 8) | low_byte; // Combine high and low bytes
}

static inline uint16_t chip_select_cpu_ram(uint16_t virtual_address) {
    // Effective address is the address within the CPU RAM range, accounting for mirroring
    uint16_t effective_address = virtual_address & 0x07FF;  // TODO: Don't use magic number

    // In order to reduce memory usage on the real system, we convert the mapped address 
    // into relative address in the CPU RAM internal memory
    uint16_t real_address = effective_address;

    #ifdef DEBUG
    printf("CPU RAM CE activated for address: 0x%04X(virtual) 0x%04X(effective) 0x%04X(real)\n", virtual_address, effective_address, real_address);
    #endif
    return real_address;
}

static inline uint16_t chip_select_ppu(uint16_t virtual_address) {
    // Effective register is the register within the PPU range, accounting for mirroring
    uint16_t effective_register = virtual_address & 0x2007;

    // In order to reduce memory usage on the real system, we convert the mapped address 
    // into relative address in the PPU internal memory
    uint16_t real_address = effective_register - PPU_REGISTER_START; // Map to 0-7
    
    #ifdef DEBUG
    printf("PPU CE activated for register: 0x%04X(virtual) 0x%04X(effective) 0x%04X(real)\n", virtual_address, effective_register, real_address);
    #endif
    return real_address;
}

static inline uint16_t chip_select_apu(uint16_t virtual_address) {
    uint16_t effective_address = virtual_address - APU_REGISTER_START; // Map to 0-31
    uint16_t real_address = effective_address; // Map to 0-31
    
    #ifdef DEBUG
    printf("APU CE activated for address: 0x%04X(virtual) 0x%04X(effective) 0x%04X(real)\n", virtual_address, effective_address, real_address);
    #endif
    return real_address;
}

static inline uint16_t chip_select_expansion_rom(uint16_t virtual_address) {
    // APU I/O must be handled in the expansion ROM range, so we need to check if the address falls within APU registers
    if (virtual_address >= APU_REGISTER_START && virtual_address <= APU_REGISTER_END) {
        return chip_select_apu(virtual_address);
    } else {
        uint16_t effective_address = virtual_address - APU_REGISTER_START;
        uint16_t real_address = effective_address; // Map to 0-8191
        
        #ifdef DEBUG
        printf("Expansion ROM CE activated for address: 0x%04X(virtual) 0x%04X(effective) 0x%04X(real)\n", virtual_address, effective_address, real_address);
        #endif
        return real_address;
    }
}

static inline uint16_t chip_select_sram(uint16_t virtual_address) {
    // Effective address is the address within the SRAM range
    uint16_t effective_address = virtual_address - SRAM_START;

    // In order to reduce memory usage on the real system, we convert the mapped address 
    // into relative address in the SRAM internal memory
    uint16_t real_address = effective_address;

    #ifdef DEBUG
    printf("SRAM CE activated for address: 0x%04X(virtual) 0x%04X(effective) 0x%04X(real)\n", virtual_address, effective_address, real_address);
    #endif
    return real_address;
}

static inline uint16_t chip_select_prg_rom(uint16_t virtual_address) {
    // Effective address is the address within the PRG ROM range
    uint16_t effective_address = virtual_address - PRG_ROM_START;

    // In order to reduce memory usage on the real system, we convert the mapped address 
    // into relative address in the PRG ROM internal memory
    uint16_t real_address = effective_address;

    #ifdef DEBUG
    printf("PRG ROM CE activated for address: 0x%04X(virtual) 0x%04X(effective) 0x%04X(real)\n", virtual_address, effective_address, real_address);
    #endif
    return real_address;
}

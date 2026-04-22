#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "memory.h"

typedef enum {
    CE_CPU_RAM = 0,
    CE_PPU = 1,
    CE_EXPANSION = 2,   // APU should be handled here
    CE_SRAM = 3,
    CE_PRG_ROM0 = 4,
    CE_PRG_ROM1 = 5,
    CE_PRG_ROM2 = 6,
    CE_PRG_ROM3 = 7
} ce_lines_t;

/**
 * @brief The NES motherboard has multiple devices sharing the same CPU bus:
 * - PRG ROM
 * - CPU RAM
 * - PPU registers
 * - APU I/O
 */
typedef struct {
    // We might need 4 bits since inactive state "0" is the same as CPU RAM
    ce_lines_t CE:3; // Chip Enable line (3 bits for 8 devices)
    memory_t* ptr_mem; // Pointer to the memory structure for address mapping
} m74ls138_t;

int m74ls138_init(m74ls138_t* dev, memory_t* mem);

// $0000-$07FF - 2KB internal RAM
#define CPU_RAM_START 0x0000        // 000 0000000000000
// $0800-$1FFF - Mirrors 1-3 of $0000-$07FF
#define CPU_RAM_END   0x1FFF        // 000 1111111111111
// $2000-$2007 - 8 B PPU registers
#define PPU_REGISTER_START 0x2000   // 001 0000000000000
// $2008-$3FFF - Repeats PPU registers every 8 bytes
#define PPU_REGISTER_END   0x3FFF   // 001 1111111111111
// $4000-$401F - 32B APU and I/O registers
#define APU_REGISTER_START 0x4000   // 010 0000000000000
#define APU_REGISTER_END   0x401F   // 010 0000000011111 
// $4020-$5FFF - 8KB expansion ROM
#define EXPANSION_ROM_START 0x4020  // 010 0000000100000
#define EXPANSION_ROM_END   0x5FFF  // 010 1111111111111
// $6000-$7FFF - 8KB SRAM battery-backed RAM (cartridge-dependent)
#define SRAM_START 0x6000           // 011 0000000000000
#define SRAM_END   0x7FFF           // 011 1111111111111
// $8000-$FFFF - 32KB PRG ROM
#define PRG_ROM_START 0x8000        // 100 0000000000000
#define PRG_ROM_END   0xFFFF        // 111 1111111111111

/**
Main flow is:
1. Looking at the CPU address pins (A13-A15)
2. Determining which chip should respond
3. Activating its Chip Enable (CE) while tri-stating all the other devices
 */

// Returns the hardware address
uint16_t decode_address(m74ls138_t* dev, uint16_t emulated_address);

// Prototypes
// address is the emulation address, which will be decoded to the real hardware address in the memory bus
void bus_write_byte(m74ls138_t* dev, uint16_t address, uint8_t value);
uint8_t bus_read_byte(m74ls138_t* dev, uint16_t address);
void bus_write_word(m74ls138_t* dev, uint16_t address, uint16_t value);
uint16_t bus_read_word(m74ls138_t* dev, uint16_t address);

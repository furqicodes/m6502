#pragma once

#include <stdint.h>
#include "program.h"

#define MEMORY_MAX_SIZE 1024 * 64

// Memory banks constants
#define M6502_RAM_SIZE 0x800        // 2KB internal RAM
#define PPU_REGISTERS_SIZE 0x8      // 8 PPU registers
#ifdef NES_USE_EXPANSION_ROM
    #define EXPANSION_ROM_SIZE 0x2000   // 8KB expansion ROM    $4020-$5FFF
#else
    #define EXPANSION_ROM_SIZE 0x0000   // No expansion ROM
#endif
#define SRAM_SIZE 0x2000            // 8KB SRAM             $6000-$7FFF
// #define PRG_ROM_SIZE 0x8000         // 32KB PRG ROM         $8000-$FFFF
#define TOTAL_MEMORY_SIZE \
    (M6502_RAM_SIZE + PPU_REGISTERS_SIZE + EXPANSION_ROM_SIZE + SRAM_SIZE)


typedef struct {
    uint8_t data[TOTAL_MEMORY_SIZE];
} memory_t;

int memory_init(memory_t *mem);

uint8_t memory_get(memory_t *mem, uint16_t address);

void memory_set(memory_t *mem, uint16_t address, uint8_t value);
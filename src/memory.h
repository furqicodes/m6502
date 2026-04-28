#pragma once

#include <stdint.h>

#define MEMORY_MAX_SIZE 1024 * 64

// Memory banks constants
#define M6502_RAM_SIZE 2048           // 2KB internal RAM
#define PPU_REGISTERS_SIZE 8        // 8 PPU registers
#define EXPANSION_ROM_SIZE 8192     // 8KB expansion ROM
#define SRAM_SIZE 8192              // 8KB SRAM
#define PRG_ROM_SIZE 32768          // 32KB PRG ROM
#define TOTAL_MEMORY_SIZE \
    (M6502_RAM_SIZE + PPU_REGISTERS_SIZE + EXPANSION_ROM_SIZE + SRAM_SIZE + PRG_ROM_SIZE)


typedef struct {
    uint8_t data[TOTAL_MEMORY_SIZE];
} memory_t;

int memory_init(memory_t *mem);

uint8_t memory_get(memory_t *mem, uint16_t address);

void memory_set(memory_t *mem, uint16_t address, uint8_t value);
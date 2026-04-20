#pragma once

#include <stdint.h>

#define MEMORY_MAX_SIZE 1024 * 64

typedef struct {
    uint8_t data[MEMORY_MAX_SIZE];
} memory_t;

int memory_init(memory_t *mem);

uint8_t memory_get(memory_t *mem, uint16_t address);

void memory_set(memory_t *mem, uint16_t address, uint8_t value);
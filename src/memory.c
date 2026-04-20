
#include "memory.h"

int memory_init(memory_t *mem) {
    for (int i = 0; i < MEMORY_MAX_SIZE; i++) {
        mem->data[i] = 0;
    }
    return 0;
}

uint8_t memory_get(memory_t *mem, uint16_t address) {
    return mem->data[address];
}

void memory_set(memory_t *mem, uint16_t address, uint8_t value) {
    mem->data[address] = value;
}
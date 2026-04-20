#include <stdio.h>
#include <stdint.h>

#include "memory.h"
#include "olc6502.h"

void print_cpu_status(olc6502_t* cpu);

static memory_t mem = { .data = {0} }; // Initialize memory with zeros to prevent uninitialized access warnings

int main(void)
{
    if (!memory_init(&mem)) {
        printf("%d bytes of memory initialized with zeros\n", sizeof(mem.data));
    }

    olc6502_t cpu;
    if (!olc6502_init(&cpu)) {
        printf("CPU initialized to power-up state\n");
    }

    print_cpu_status(&cpu);

    printf("Hello, World!\n");
    return 0;
}


void print_cpu_status(olc6502_t* cpu) {
    printf("A: 0x%02X, X: 0x%02X, Y: 0x%02X, SP: 0x%02X, PS: 0b%08b, PC: 0x%04X\n",
           cpu->A, cpu->X, cpu->Y, cpu->SP, cpu->PS, cpu->PC);
}
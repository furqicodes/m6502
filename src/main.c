#include <stdio.h>
#include <stdint.h>

#include "memory.h"
#include "olc6502.h"
#include "include/olc6502_constants.h"
#include "ti74ls138.h"

#include "shell.h"

void print_cpu_status(olc6502_t* cpu);
int clock_command(int argc, char **argv);
int print_command(int argc, char **argv);
int reset_command(int argc, char **argv);
int inspect_memory_command(int argc, char **argv);
SHELL_COMMAND(clock, "Run the CPU for a specified number of cycles", clock_command);
SHELL_COMMAND(print, "Print the current CPU status", print_command);
SHELL_COMMAND(reset, "Reset the CPU to its initial state", reset_command);
SHELL_COMMAND(inspect, "Inspect memory at a specified address range", inspect_memory_command);

static memory_t mem; // Initialize memory with zeros to prevent uninitialized access warnings
static m74ls138_t ce;
static olc6502_t cpu; // Declare CPU instance

int main(void)
{
    if (!memory_init(&mem)) {
        printf("%d bytes of memory initialized with zeros\n", sizeof(mem.data));
    }

    if (!m74ls138_init(&ce, &mem)) {
        printf("74LS138 decoder initialized and connected to memory bus\n");
    }

    if (!olc6502_init(&cpu, &ce)) {
        printf("CPU initialized to power-up state\n");
    }

    bus_write_byte(&ce, 0xFFFC, 0x00);
    bus_write_byte(&ce, 0xFFFD, 0x80);

    olc6502_reset(&cpu, &(int32_t){9}); // Reset the CPU to set the PC to the reset vector
    char buffer[SHELL_DEFAULT_BUFSIZE];
    shell_run(NULL, buffer, SHELL_DEFAULT_BUFSIZE);

    print_cpu_status(&cpu);

    printf("Hello, World!\n");
    return 0;
}


void print_cpu_status(olc6502_t* cpu) {
    printf("A: 0x%02X, X: 0x%02X, Y: 0x%02X, SP: 0x%02X, PS: 0b%08b, PC: 0x%04X\n",
           cpu->A, cpu->X, cpu->Y, cpu->SP, cpu->PS.value, cpu->PC);
}

int clock_command(int argc, char **argv) {
    if (argc != 2) {
        printf("Usage: clock <cycles>\n");
        return -1;
    }
    uint32_t cycles = (uint32_t)atoi(argv[1]);
    int actual_cycles = olc6502_clock(&cpu, cycles);
    printf("Requested %u cycles, executed %d cycles\n", cycles, actual_cycles);
    print_cpu_status(&cpu);
    return 0;
}

int print_command(int argc, char **argv) {
    if (argc != 1) {
        printf("Usage: print\n");
        return -1;
    }
    (void)argv; // Unused parameter
    print_cpu_status(&cpu);
    return 0;
}

int reset_command(int argc, char **argv) {
    if (argc != 1) {
        printf("Usage: reset\n");
        return -1;
    }
    (void)argv; // Unused parameter
    int32_t cycles = 9; // Reset takes 7 cycles, plus 2 cycles for the initial fetch of the reset vector
    olc6502_reset(&cpu, &cycles);
    print_cpu_status(&cpu);
    return 0;
}

int inspect_memory_command(int argc, char **argv) {
    if (argc != 3) {
        printf("Usage: inspect <start_address> <end_address>\n");
        return -1;
    }
    uint16_t start = (uint16_t)strtoul(argv[1], NULL, 0);
    uint16_t end = (uint16_t)strtoul(argv[2], NULL, 0);
    if (start > end) {
        printf("Invalid address range. Start should be <= end and both should be < %d\n", MEMORY_MAX_SIZE);
        return -1;
    }
    printf("0x%04X:\t", start);
    for (uint32_t addr = start; addr <= end; addr++) {
        printf("0x%02X ", bus_read_byte(&ce, addr));
    }
    printf("\n");
    return 0;
}
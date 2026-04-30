#include <stdio.h>
#include <stdint.h>

#include "memory.h"
#include "olc6502.h"
#include "include/olc6502_constants.h"
#include "ti74ls138.h"

// ACIA registers
#define ACIA_DATA       0x4100
#define ACIA_STATUS     0x4101

#include "shell.h"

void print_cpu_status(olc6502_t* cpu);
int clock_command(int argc, char **argv);
int print_command(int argc, char **argv);
int reset_command(int argc, char **argv);
int inspect_memory_command(int argc, char **argv);
int run_command(int argc, char **argv);
int wozmon_command(int argc, char **argv);
SHELL_COMMAND(clock, "Run the CPU for a specified number of cycles", clock_command);
SHELL_COMMAND(print, "Print the current CPU status", print_command);
SHELL_COMMAND(reset, "Reset the CPU to its initial state", reset_command);
SHELL_COMMAND(inspect, "Inspect memory at a specified address range", inspect_memory_command);
SHELL_COMMAND(run, "Run the CPU until it halts or triggers an interrupt", run_command);
SHELL_COMMAND(wozmon, "Enter WOZMON monitor mode", wozmon_command);

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
    int cycles = atoi(argv[1]);
    int actual_cycles = olc6502_clock(&cpu, cycles);
    printf("Requested %d cycles, executed %d cycles\n", cycles, actual_cycles);
    print_cpu_status(&cpu);
    return 0;
}

int run_command(int argc, char **argv) {
    if (argc != 1) {
        printf("Usage: run\n");
        return -1;
    }
    (void)argv; // Unused parameter
    int total_cycles = 0;
    uint16_t irq_address = bus_read_word(&ce, IRQ_VECTOR);
    while (cpu.PC != irq_address) { // Run until an IRQ is triggered, which indicates the end of the program
        int cycles = olc6502_clock(&cpu, 1); // Run one cycle at a time to allow for interrupts and other events
        total_cycles += cycles;
        if (cycles == 0) {
            break; // Stop if the CPU is halted or waiting for an event
        }
    }
    uint8_t is_break = (cpu.SP == STACK_BASE - 3) ? 0 :
        ((bus_read_byte(&ce, cpu.SP + 1) & 0x10) != 0); // Check the Break Flag in the last pushed stack frame
    printf("CPU halted after executing %d cycles. Break Flag: %d\n", total_cycles, is_break);
    print_cpu_status(&cpu);
    return 0;
}

int wozmon_command(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: wozmon <read/write> <empty/data>\n");
        return -1;
    }
    uint16_t irq_address = bus_read_word(&ce, IRQ_VECTOR);
    
    if (strcmp(argv[1], "read") == 0) {

        uint8_t mon_data = bus_read_byte(&ce, ACIA_DATA); // Monitor address for user input to step through cycles
        uint8_t buf[0x0100] = {0}; // Buffer to store user input, initialized to zeros
        size_t input_index = 0;
        uint8_t rx_done = 0;
        
        while (cpu.PC != irq_address && rx_done == 0) { // Run until an IRQ is triggered, which indicates the end of the program
            // Scan for user input to step through each cycle, allowing for interrupts and other events to be processed
            uint8_t new_mon_data = bus_read_byte(&ce, ACIA_DATA);
            // Check if new data received
            if (new_mon_data != mon_data) {
                buf[input_index] = new_mon_data;
                mon_data = new_mon_data;
                input_index++;
                // If CR received, print the buffer and reset it for the next input
                if (new_mon_data == 0x0D) {
                    printf("WOZMON: %s\n", buf);
                    input_index = 0; // Reset input index for the next command
                    for (size_t i = 0; i < sizeof(buf); i++) {
                        buf[i] = '\0'; // Clear the buffer after writing to memory
                    }
                    rx_done = 1; // Set the receive done flag
                }
            }
            olc6502_clock(&cpu, 1);
        }
    }

    else if (strcmp(argv[1], "write") == 0) {
        char* input = (argc >= 3) ? argv[2] : ""; // Get the input string from command line arguments, default to empty string if not provided
        for (size_t i = 0; i < sizeof(input) && input[i] != '\0'; i++) {
            bus_write_byte(&ce, ACIA_DATA, input[i]); // Store user input in memory in ACIA_DATA
            bus_write_byte(&ce, ACIA_STATUS, 0x10); // Signal the CPU that there is new data ready to be processed
            olc6502_clock(&cpu, 1); // Run one cycle to allow the CPU to process the new input
        }
    }

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
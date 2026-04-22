#include <stdio.h>
#include <assert.h>

#include "ti74ls138.h"
#include "memory.h"

static m74ls138_t ce;
static memory_t mem;

int main(void) {
    printf("74LS138 3-to-8 Line Decoder/Demultiplexer\n");

    if (memory_init(&mem) != 0) {
        printf("Failed to initialize memory\n");
        return -1;
    }

    if (m74ls138_init(&ce, &mem) != 0) {
        printf("Failed to initialize 74LS138 decoder\n");
        return -1;
    }

    // CPU Base RAM tests
    uint16_t test_address_cpu_ram = 0x000A;
    uint8_t test_value = 0x42;
    memory_set(ce.ptr_mem, decode_address(&ce, test_address_cpu_ram), test_value); // Write to CPU RAM
    assert(memory_get(&mem, test_address_cpu_ram) == test_value); // Read back from CPU RAM
    memory_set(ce.ptr_mem, test_address_cpu_ram, test_value + 1); // Write to mirrored address
    assert(memory_get(&mem, decode_address(&ce, test_address_cpu_ram + 0x0800)) == test_value + 1); // Read back from mirrored address

    // PPU register tests
    uint16_t test_address_ppu = 0x2002;
    uint8_t test_value_ppu = 0x3;
    memory_set(ce.ptr_mem, decode_address(&ce, test_address_ppu), test_value_ppu); // Write to PPU register
    assert(memory_get(&mem, 2048 + 2) == test_value_ppu); // Read back from PPU register
    memory_set(ce.ptr_mem, 2048 + 2, test_value_ppu + 1); // Write to mirrored PPU register
    assert(memory_get(&mem, decode_address(&ce, test_address_ppu + 0x0008)) == test_value_ppu + 1); // Read back from mirrored PPU register

    // APU I/O and Expansion ROM address tests
    decode_address(&ce, 0x4005); // 2048 + 8 + 5 = 2061
    decode_address(&ce, 0x4021); // 2048 + 8 + 32 + 1 = 2089

    // SRAM tests
    decode_address(&ce, 0x6000); // 2048 + 8 + 8192 = 10248
    
    // PRG ROM tests
    decode_address(&ce, 0x8000); // 2048 + 8 + 8192 + 8192 = 18440

    // Bus Write/Read tests
    bus_write_byte(&ce, 0x000B, 0x55);
    assert(bus_read_byte(&ce, 0x000B) == 0x55);
    bus_write_byte(&ce, 0x8000, 0x56); 
    assert(bus_read_byte(&ce, 0x8000) == 0x56);
    bus_write_word(&ce, 0x09FD, 0x1234);
    assert(bus_read_word(&ce, 0x01FD) == 0x1234);

    return 0;
}
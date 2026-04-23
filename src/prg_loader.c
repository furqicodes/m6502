#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "../src/ti74ls138.h"
#include "../src/memory.h"

#define BUF_SIZE 64

typedef struct {
    uint16_t load_address;
    uint16_t size;
    uint8_t* data;
} prg_segment_t;

int load_prg_file(const char* fileName, m74ls138_t* ce);

int write_prg_file(const char* fileName, prg_segment_t* segments, size_t num_segments);

#ifdef DEBUGLOAD
int main(void) {
    printf("PRG Loader Test Program\n");
    m74ls138_t ce; // Declare a 74LS138 instance for address decoding
    memory_t mem; // Declare a memory instance for the CPU bus

    memory_init(&mem);
    m74ls138_init(&ce, &mem);

    const char fileName[] = "output.txt";
    uint8_t segment0[] = {0xDE, 0xAD, 0xBE, 0xEF, 0x00};
    uint8_t segment1[] = {0xBA, 0xAD, 0xF0, 0x0D, 0x01, 0x02, 0x03, 0x04};
    uint8_t segment2[] = {0xFE, 0xED, 0xFA, 0xCE, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A};
    
    prg_segment_t segments[] = {
        {0x8000, sizeof(segment0), segment0},
        {0x9000, sizeof(segment1), segment1},
        {0xA000, sizeof(segment2), segment2}
    };
    
    int ret = write_prg_file(fileName, segments, sizeof(segments) / sizeof(segments[0]));
    
    if (ret != 0) return ret;

    ret = load_prg_file(fileName, &ce);

    return ret;
}
#endif

int load_prg_file(const char* fileName, m74ls138_t* ce) {
    char myString[BUF_SIZE];
    FILE *fptr;    
    fptr = fopen(fileName, "r");
    if (fptr == NULL) {
        fprintf(stderr, "File %s not found!\n", fileName);
        return -1;
    }
    
    while(fgets(myString, BUF_SIZE, fptr)) {
        // Get Load Address
        char *token = strtok(myString, ":");
        uint16_t loadAddr = strtol(token, NULL, 16);
        
        // Get bytes and write to memory
        token = strtok(NULL, " ");
        uint16_t offset = 0;
        while (token != NULL) {
            uint8_t data = (uint8_t)strtol(token, NULL, 16);
            uint16_t virtual_addr = loadAddr + offset;
            
            // Write the byte to the memory bus, which will be decoded by the 74LS138 to determine the actual physical address in memory
            bus_write_byte(ce, virtual_addr, data);

            token = strtok(NULL, " ");
            offset++;
            #ifdef DEBUGLOAD
            uint16_t physical_addr = decode_address(ce, virtual_addr);
            printf("0x%04X -> 0x%04X: 0x%02X ", virtual_addr, physical_addr, data);
            #endif
        }
        printf("\n");
    }
    
    fclose(fptr);
    return 0;
}

int write_prg_file(const char* fileName, prg_segment_t* segments, size_t num_segments) {
    FILE *fptr;    
    fptr = fopen(fileName, "w");
    if (fptr == NULL) {
        fprintf(stderr, "Unable to create file %s!\n", fileName);
        return -1;
    }

    for (size_t i = 0; i < num_segments; i++) {
        prg_segment_t* segment = &segments[i];
        fprintf(fptr, "0x%04X: ", segment->load_address);
        for (size_t j = 0; j < segment->size; j++) {
            fprintf(fptr, "%02X ", segment->data[j]);
        }
        fprintf(fptr, "\n");
    }
    
    fclose(fptr);
    return 0;
}

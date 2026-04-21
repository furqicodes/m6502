#include "olc6502.h"
#include "include/olc6502_constants.h"

int olc6502_init(olc6502_t* cpu) {
    cpu->A = 0;
    cpu->X = 0;
    cpu->Y = 0;
    cpu->PC = RESET_VECTOR;
    cpu->SP = DEFAULT_STACK_POINTER;
    cpu->PS.value = 0x04; // Clear all flags except the Interrupt Disable Flag
    return 0;
}

void olc6502_reset(olc6502_t* cpu) {
    cpu->PC = RESET_VECTOR;
    cpu->SP = DEFAULT_STACK_POINTER;
    cpu->PS.I = 1; // Set Interrupt Disable Flag
    cpu->PS.D = 0; // Clear Decimal Mode Flag
}

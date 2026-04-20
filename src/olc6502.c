#include "olc6502.h"

int olc6502_init(olc6502_t* cpu) {
    cpu->A = 0;
    cpu->X = 0;
    cpu->Y = 0;
    cpu->PC = 0xFFFC;
    cpu->SP = 0xFD;
    cpu->PS = 0 | INTERRUPT_DISABLE_FLAG;
    return 0;
}

void olc6502_reset(olc6502_t* cpu) {
    cpu->PC = 0xFFFC;
    cpu->SP = 0xFD;
    // Enable Interrupt Disable Flag and clear Decimal Mode Flag
    cpu->PS |= INTERRUPT_DISABLE_FLAG;
    cpu->PS &= ~DECIMAL_MODE_FLAG;
}


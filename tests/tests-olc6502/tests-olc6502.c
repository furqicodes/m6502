#include "embUnit.h"
#include "../../../../src/memory.h"
#include "../../../../src/olc6502.h"
#include "tests-olc6502.h"

#include <stdint.h>
#include <stdbool.h>

static memory_t mem; // Global memory instance for tests

static void set_up(void)
{
    memory_init(&mem); // Initialize memory before each test
}

static void tear_down(void)
{
    
}

static void test_olc6502_initialize(void) {
    olc6502_t cpu;
    uint8_t startup_flags = 0b00000100;
    uint8_t expected_A = 0;
    uint8_t expected_X = 0;
    uint8_t expected_Y = 0;
    uint8_t expected_SP = 0xFD;
    uint16_t expected_PC = 0xFFFC;

    olc6502_init(&cpu);

    TEST_ASSERT_EQUAL_INT(startup_flags, cpu.PS);
    TEST_ASSERT_EQUAL_INT(expected_A, cpu.A);
    TEST_ASSERT_EQUAL_INT(expected_X, cpu.X);
    TEST_ASSERT_EQUAL_INT(expected_Y, cpu.Y);
    TEST_ASSERT_EQUAL_INT(expected_SP, cpu.SP);
    TEST_ASSERT_EQUAL_INT(expected_PC, cpu.PC);
}

static void test_olc6502_reset(void) {
    olc6502_t cpu;
    cpu.PS = 0xCC;
    cpu.A = 0x01;
    cpu.X = 0x02;
    cpu.Y = 0x03;
    cpu.SP = 0xFD;
    cpu.PC = 0xFFF0;
    uint8_t expected_flags = (cpu.PS | INTERRUPT_DISABLE_FLAG) & ~DECIMAL_MODE_FLAG;
    uint8_t expected_A = cpu.A; // A should remain unchanged on reset
    uint8_t expected_X = cpu.X; // X should remain unchanged on reset
    uint8_t expected_Y = cpu.Y; // Y should remain unchanged on reset
    uint8_t expected_SP = 0xFD;
    uint16_t expected_PC = 0xFFFC;
    
    olc6502_reset(&cpu);
    
    TEST_ASSERT_EQUAL_INT(expected_flags, cpu.PS);
    TEST_ASSERT_EQUAL_INT(expected_A, cpu.A);
    TEST_ASSERT_EQUAL_INT(expected_X, cpu.X);
    TEST_ASSERT_EQUAL_INT(expected_Y, cpu.Y);
    TEST_ASSERT_EQUAL_INT(expected_SP, cpu.SP);
    TEST_ASSERT_EQUAL_INT(expected_PC, cpu.PC);
}

Test *tests_olc6502_tests(void)
{
    EMB_UNIT_TESTFIXTURES(fixtures) {
        new_TestFixture(test_olc6502_initialize),
        new_TestFixture(test_olc6502_reset),
    };

    EMB_UNIT_TESTCALLER(olc6502_initial_tests, set_up, tear_down, fixtures);
    /* set up and tear down function can be NULL if omitted */

    return (Test *)&olc6502_initial_tests;
}

void tests_olc6502(void)
{
    TESTS_RUN(tests_olc6502_tests());
}
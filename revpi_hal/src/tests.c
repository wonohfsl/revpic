#include "hal.h"
#include <stdio.h>

// Basic read/write test
static int test_read_write_basic(void) {
    hal_write_byte(10, 0x55);
    uint8_t v = hal_read_byte(10);

    return (v == 0x55) ? 0 : 1;
}

// Bounds check test
static int test_bounds_check(void) {
    hal_write_byte(5000, 0xAA);   // out of range in sim
    uint8_t v = hal_read_byte(5000);
    (void)v; // just ensure no crash

    return 0;
}

int run_hal_tests(void) {
    int fails = 0;

    if (test_read_write_basic() != 0) {
        printf("test_read_write_basic FAILED\n");
        fails++;
    } else {
        printf("test_read_write_basic PASSED\n");
    }
    if (test_bounds_check() != 0) {
        printf("test_bounds_check FAILED\n");
        fails++;
    } else {
        printf("test_bounds_check PASSED\n");
    }

    return fails;
}

void run_mock_console(void);  // from mock_io.c

#ifdef HAL_TEST_MAIN
int main() {
    if (hal_init() != 0) {
        fprintf(stderr, "HAL init failed\n");
        return 1;
    }

    int fails = run_hal_tests();
    printf("HAL tests: %s\n", fails == 0 ? "ALL PASSED" : "SOME FAILED");
    run_mock_console();
    hal_close();

    return fails ? 1 : 0;
}
#endif

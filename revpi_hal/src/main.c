#include "hal.h"
#include <stdio.h>

int main() {
    if (hal_init() != 0) {
        fprintf(stderr, "HAL init failed\n");
        return 1;
    }

    // Example: read DI1 (offset 0) via HAL
    uint8_t hdi1 = 0;
    hdi1 = hal_read_byte(0);
    printf("DI1 = %u\n", hdi1);

    // Example: write DO3 (offset 34) via HAL
    hal_write_byte(34, 1);
    printf("DO3 set to 1\n");

    // Example: read DI1 (offset 0) via HAL
    hdi1 = hal_read_byte(0);
    printf("DI1 = %u\n", hdi1);

    hal_close();
    return 0;
}

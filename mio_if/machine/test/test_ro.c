/**
 * @file test_ro.c
 * @brief Unit test for the RevPi RO Hardware Abstraction Layer (HAL).
 *
 * This test exercises:
 *   - Relay output readback (RO1–RO4)
 *   - Relay output write operations
 *
 * All offsets and bit positions come from ro_addr.h.
 */

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

#include "ro.h"
#include "piControl.h"
#include "piControlIf.h"

#ifndef RO_H
#error "WARNING: ro.h not included!"
#endif

static void print_ro_info(void)
{
    printf("====== RO Module ======\n\n");

    printf("=== Relay Outputs ===\n");
    for (int ch = 1; ch <= 4; ch++) {
        int offset, bit, len;
        if (ro_get_addr(ch, &offset, &bit, &len) == 0) {
            printf("RO%d: offset=%d bit=%d len=%d\n", ch, offset, bit, len);
        } else {
            printf("RO%d: ERROR retrieving address\n", ch);
        }
    }
    printf("\n");
}

static void test_relay_rw(void)
{
    printf("=== Relay Output Read/Write Test ===\n");

    for (int ch = 1; ch <= 4; ch++) {
        printf("\n-- Testing RO%d --\n", ch);

        int before = ro_get_ro(ch);
        printf("Initial state: %d\n", before);

        printf("Setting RO%d = 1...\n", ch);
        ro_set_ro(ch, 1);
        //usleep(20000);
		sleep(5);
        printf("Readback: %d\n", ro_get_ro(ch));

        printf("Setting RO%d = 0...\n", ch);
        ro_set_ro(ch, 0);
        //usleep(20000);
		sleep(5);
        printf("Readback: %d\n", ro_get_ro(ch));
    }

    printf("\nRelay test complete.\n\n");
}

int main(void)
{
    printf("Opening /dev/piControl0...\n");

    if (ro_init() < 0) {
        printf("ERROR: Failed to open piControl\n");
        return 1;
    }

    print_ro_info();
    test_relay_rw();

    return 0;
}

/**
 * @file test_machine.c
 * @brief Test program for mid-level motion I/O functions (motion.c).
 *
 * This test verifies that the mid-level abstraction correctly calls
 * into the low-level HAL (mio.c, ro.c) and that channel mappings are valid.
 */

#include <stdio.h>
#include <unistd.h>

#include "motion.h"
#include "mio.h"
#include "ro.h"

static void test_inputs(void)
{
    printf("=== Testing Digital Inputs ===\n");

    int estop = ReadEStopButton();
    printf("E-STOP: %d\n", estop);

    int home_rot = ReadHomeRotate();
    printf("Home Rotate: %d\n", home_rot);

    int home_tilt = ReadHomeTilt();
    printf("Home Tilt: %d\n", home_tilt);

    printf("\n=== Testing Analog Inputs ===\n");

    int tilt_pos = ReadTiltPosition();
    printf("Tilt Position (AI1): %d\n", tilt_pos);
}

static void test_rotate(void)
{
    printf("\n=== Testing Rotate Actuator ===\n");

    printf("Rotate CW ON\n");
    RelayRotate(1, 1);
    sleep(1);

    printf("Rotate CW OFF\n");
    RelayRotate(1, 0);
    sleep(1);

    printf("Rotate CCW ON\n");
    RelayRotate(0, 1);
    sleep(1);

    printf("Rotate CCW OFF\n");
    RelayRotate(0, 0);
}

static void test_tilt(void)
{
    printf("\n=== Testing Tilt Actuator ===\n");

    printf("Tilt UP ON\n");
    RelayTilt(1, 1);
    sleep(1);

    printf("Tilt UP OFF\n");
    RelayTilt(1, 0);
    sleep(1);

    printf("Tilt DOWN ON\n");
    RelayTilt(0, 1);
    sleep(1);

    printf("Tilt DOWN OFF\n");
    RelayTilt(0, 0);
}

int main(void)
{
    printf("=== Initializing HAL ===\n");

    if (mio_init() < 0) {
        printf("ERROR: mio_init() failed\n");
        return 1;
    }

    if (ro_init() < 0) {
        printf("ERROR: ro_init() failed\n");
        return 1;
    }

    printf("HAL initialized.\n\n");

    test_inputs();
    test_rotate();
    test_tilt();

    printf("\n=== Test Complete ===\n");
    return 0;
}

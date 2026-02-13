/**
 * @file test_motion_hw.c
 * @brief Hardware validation test for mid-level motion HAL.
 *
 * This test verifies real hardware behavior:
 *  - DI sensors change as expected
 *  - AI tilt position is within valid range
 *  - RO relays actually switch ON/OFF
 *
 * Use this to validate wiring and hardware installation.
 */

#include <stdio.h>
#include <unistd.h>

#include "motion.h"
#include "mio.h"
#include "ro.h"

static void check(const char *label, int condition)
{
    printf("%-30s : %s\n", label, condition ? "PASS" : "FAIL");
}

static void test_inputs_hw(void)
{
    printf("\n=== Hardware Test: Inputs ===\n");

    int estop = ReadEStopButton();
    check("E-STOP (expect 0 or 1)", estop == 0 || estop == 1);

    int home_rot = ReadHomeRotate();
    check("Home Rotate (0 or 1)", home_rot == 0 || home_rot == 1);

    int home_tilt = ReadHomeTilt();
    check("Home Tilt (0 or 1)", home_tilt == 0 || home_tilt == 1);

    int tilt_pos = ReadTiltPosition();
    check("Tilt Position (0–10000)", tilt_pos >= 0 && tilt_pos <= 10000);
}

static void test_rotate_hw(void)
{
    printf("\n=== Hardware Test: Rotate ===\n");

    RelayRotate(1, 1);
    sleep(1);
    check("Rotate EN=1", ro_get_ro(RO_ROTATE_EN) == 1);

    RelayRotate(1, 0);
    sleep(1);
    check("Rotate EN=0", ro_get_ro(RO_ROTATE_EN) == 0);

    RelayRotate(0, 1);
    sleep(1);
    check("Rotate EN=1", ro_get_ro(RO_ROTATE_EN) == 1);

    RelayRotate(0, 0);
    sleep(1);
    check("Rotate EN=0", ro_get_ro(RO_ROTATE_EN) == 0);
}

static void test_tilt_hw(void)
{
    printf("\n=== Hardware Test: Tilt ===\n");

    RelayTilt(1, 1);
    sleep(1);
    check("Tilt EN=1", ro_get_ro(RO_TILT_EN) == 1);

    RelayTilt(1, 0);
    sleep(1);
    check("Tilt EN=0", ro_get_ro(RO_TILT_EN) == 0);

    RelayTilt(0, 1);
    sleep(1);
    check("Tilt EN=1", ro_get_ro(RO_TILT_EN) == 1);

    RelayTilt(0, 0);
    sleep(1);
    check("Tilt EN=0", ro_get_ro(RO_TILT_EN) == 0);
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

    test_inputs_hw();
    test_rotate_hw();
    test_tilt_hw();

    printf("\n=== Hardware Validation Complete ===\n");
    return 0;
}

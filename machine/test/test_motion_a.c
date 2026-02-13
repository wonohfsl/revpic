/**
 * @file test_motion_a.c
 * @brief Assertion-based unit tests for mid-level motion HAL.
 *
 * These tests verify that motion.c correctly calls into the low-level
 * HAL and that return values are within valid ranges.
 */

#include <stdio.h>
#include <assert.h>

#include "motion.h"
#include "mio.h"
#include "ro.h"

static void test_inputs(void)
{
    printf("=== test_inputs() ===\n");

    int estop = ReadEStopButton();
    assert(estop == 0 || estop == 1 || estop == -1);

    int home_rot = ReadHomeRotate();
    assert(home_rot == 0 || home_rot == 1 || home_rot == -1);

    int home_tilt = ReadHomeTilt();
    assert(home_tilt == 0 || home_tilt == 1 || home_tilt == -1);

    int tilt_pos = ReadTiltPosition();
    assert(tilt_pos >= -1);   // -1 = error, otherwise 0–10000

    printf("Inputs OK.\n");
}

static void test_rotate(void)
{
    printf("=== test_rotate() ===\n");

    RelayRotate(1, 1);
    int en = ro_get_ro(RO_ROTATE_EN);
    assert(en == 1);

    RelayRotate(1, 0);
    en = ro_get_ro(RO_ROTATE_EN);
    assert(en == 0);

    RelayRotate(0, 1);
    en = ro_get_ro(RO_ROTATE_EN);
    assert(en == 1);

    RelayRotate(0, 0);
    en = ro_get_ro(RO_ROTATE_EN);
    assert(en == 0);

    printf("Rotate OK.\n");
}

static void test_tilt(void)
{
    printf("=== test_tilt() ===\n");

    RelayTilt(1, 1);
    int en = ro_get_ro(RO_TILT_EN);
    assert(en == 1);

    RelayTilt(1, 0);
    en = ro_get_ro(RO_TILT_EN);
    assert(en == 0);

    RelayTilt(0, 1);
    en = ro_get_ro(RO_TILT_EN);
    assert(en == 1);

    RelayTilt(0, 0);
    en = ro_get_ro(RO_TILT_EN);
    assert(en == 0);

    printf("Tilt OK.\n");
}

int main(void)
{
    printf("=== Initializing HAL ===\n");

    assert(mio_init() == 0);
    assert(ro_init() == 0);

    test_inputs();
    test_rotate();
    test_tilt();

    printf("\nAll tests passed.\n");
    return 0;
}

/**
 * @file test_control_rotate.c
 * @brief Test program for validating the non-blocking rotate control engine.
 *
 * Test sequence:
 *   1) RotateHome (CW until HOME sensor detects metal)
 *   2) Wait 5 seconds
 *   3) RotateOne (CW)
 *   4) Wait 5 seconds
 *   5) RotateOne (CCW)
 *   6) Wait 5 seconds
 *   7) RotateMoveToDegree (CW, 90)
 *   8) Wait 5 seconds
 *   9) RotateMoveToDegree (CW, 10)
 *  10) Wait 5 seconds
 *  11) RotateMoveToDegree (CW, 10)
 *  12) Wait 5 seconds
 *  13) RotateMoveToDegree (CW, 10)
 */

#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include "mio.h"
#include "ro.h"
#include "motion.h"
#include "machine_state.h"
#include "control_rotate.h"

/* -------------------------------------------------------------------------
 * Helper: Print timestamp, state, and HOME sensor info
 * ------------------------------------------------------------------------- */

/**
 * @brief Print current status for rotation axis.
 *
 * @param label Status label for logs.
 */
static void PrintStatus(const char *label)
{
    time_t now = time(NULL);
    struct tm *tm_now = localtime(&now);
    int home = ReadHomeRotate();
    int raw = mio_get_di(DI_PROXI_ROTATE);

    printf("[%02d:%02d:%02d] %-20s  state=%d  home=%d  home_raw=%d\n",
           tm_now->tm_hour,
           tm_now->tm_min,
           tm_now->tm_sec,
           label ? label : "status",
           g_machine.rotate_state,
           home,
           raw);
}

/**
 * @brief Wait for a number of seconds while printing status.
 *
 * @param seconds Number of seconds to wait.
 * @param label Label to print during wait.
 */
static void WaitSeconds(int seconds, const char *label)
{
    for (int i = 0; i < seconds; i++) {
        sleep(1);
        PrintStatus(label);
    }
}

/**
 * @brief Run the non-blocking RotateHome sequence.
 *
 * @param tick_ms Control tick period in milliseconds.
 * @return 0 on success, -1 on failure.
 */
static int RunRotateHome(int tick_ms)
{
    RotateResult_t r = ControlRotate_RotateHome();
    if (r == ROTATE_ERROR) return -1;
    if (r == ROTATE_OK) return 0;

    while (1) {
        r = ControlRotate_ServiceHome();
        PrintStatus("RotateHome");

        if (r == ROTATE_OK) return 0;
        if (r == ROTATE_STOPPED || r == ROTATE_ERROR) return -1;

        usleep(tick_ms * 1000);
    }
}

/**
 * @brief Run the non-blocking RotateOne sequence.
 *
 * @param dir Rotation direction.
 * @param tick_ms Control tick period in milliseconds.
 * @return 0 on success, -1 on failure.
 */
static int RunRotateOne(RotateDirection_t dir, int tick_ms)
{
    RotateResult_t r = ControlRotate_RotateOne(dir);
    if (r == ROTATE_ERROR) return -1;
    if (r == ROTATE_OK) return 0;

    while (1) {
        r = ControlRotate_Service();
        PrintStatus("RotateOne");

        if (r == ROTATE_OK) return 0;
        if (r == ROTATE_STOPPED || r == ROTATE_ERROR) return -1;

        usleep(tick_ms * 1000);
    }
}

/**
 * @brief Run the non-blocking RotateMoveToDegree sequence.
 *
 * @param dir Rotation direction.
 * @param degrees Rotation degrees.
 * @param tick_ms Control tick period in milliseconds.
 * @return 0 on success, -1 on failure.
 */
static int RunRotateMoveToDegree(RotateDirection_t dir, float degrees,
                                 int tick_ms)
{
    RotateResult_t r = ControlRotate_RotateMoveToDegree(dir, degrees);
    if (r == ROTATE_ERROR) return -1;
    if (r == ROTATE_OK) return 0;

    while (1) {
        r = ControlRotate_Service();
        PrintStatus("RotateMove");

        if (r == ROTATE_OK) return 0;
        if (r == ROTATE_STOPPED || r == ROTATE_ERROR) return -1;

        usleep(tick_ms * 1000);
    }
}

/**
 * @brief Main entry point for rotate control test.
 *
 * @return 0 on success, non-zero on failure.
 */
int main(void)
{
    printf("=== Test: control_rotate.c (degree-based, non-blocking) ===\n");

    if (mio_init() < 0) {
        printf("ERROR: mio_init failed\n");
        return 1;
    }
    if (ro_init() < 0) {
        printf("ERROR: ro_init failed\n");
        return 1;
    }

    g_machine.pause_requested  = 0;
    g_machine.resume_requested = 0;
    g_machine.stop_requested   = 0;
    g_machine.tilt_state       = AXIS_IDLE;
    g_machine.rotate_state     = AXIS_IDLE;

    RotateCalibration_t cal = {
        .rpm              = 1.0f,
        .control_time_ms  = 100,
        .timeout_margin_ms = 5000
    };

    ControlRotate_ApplyCalibration(&cal);

    const int tick_ms = cal.control_time_ms;

    PrintStatus("Initial");

    printf("\nSTEP 1: RotateHome (CW until HOME)\n");
    printf("Touch metal to sensor to trigger HOME. ");
    printf("home_raw=1 when not touched, 0 when touched.\n");

    if (RunRotateHome(tick_ms) != 0) {
        printf("RotateHome failed\n");
        return 1;
    }

    printf("\nSTEP 2: Wait 5 seconds\n");
    WaitSeconds(5, "Wait");

    printf("\nSTEP 3: RotateOne (CW)\n");
    if (RunRotateOne(ROTATE_DIR_CW, tick_ms) != 0) {
        printf("RotateOne CW failed\n");
        return 1;
    }

    printf("\nSTEP 4: Wait 5 seconds\n");
    WaitSeconds(5, "Wait");

    printf("\nSTEP 5: RotateOne (CCW)\n");
    if (RunRotateOne(ROTATE_DIR_CCW, tick_ms) != 0) {
        printf("RotateOne CCW failed\n");
        return 1;
    }

    printf("\nSTEP 6: Wait 5 seconds\n");
    WaitSeconds(5, "Wait");

    printf("\nSTEP 7: RotateMoveToDegree (CW, 90)\n");
    if (RunRotateMoveToDegree(ROTATE_DIR_CW, 90.0f, tick_ms) != 0) {
        printf("RotateMoveToDegree 90 failed\n");
        return 1;
    }

    printf("\nSTEP 8: Wait 5 seconds\n");
    WaitSeconds(5, "Wait");

    printf("\nSTEP 9: RotateMoveToDegree (CW, 10)\n");
    if (RunRotateMoveToDegree(ROTATE_DIR_CW, 10.0f, tick_ms) != 0) {
        printf("RotateMoveToDegree 10 failed\n");
        return 1;
    }

    printf("\nSTEP 10: Wait 5 seconds\n");
    WaitSeconds(5, "Wait");

    printf("\nSTEP 11: RotateMoveToDegree (CW, 10)\n");
    if (RunRotateMoveToDegree(ROTATE_DIR_CW, 10.0f, tick_ms) != 0) {
        printf("RotateMoveToDegree 10 failed\n");
        return 1;
    }

    printf("\nSTEP 12: Wait 5 seconds\n");
    WaitSeconds(5, "Wait");

    printf("\nSTEP 13: RotateMoveToDegree (CW, 10)\n");
    if (RunRotateMoveToDegree(ROTATE_DIR_CW, 10.0f, tick_ms) != 0) {
        printf("RotateMoveToDegree 10 failed\n");
        return 1;
    }

    printf("\nTest complete\n");
    return 0;
}

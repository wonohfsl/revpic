/**
 * @file test_t_direction.c
 * @brief Direct hardware test to determine tilt actuator direction.
 *
 * This test bypasses control_tilt.c entirely and talks directly to HAL:
 *   - RelayTilt(dir, enable)
 *   - ReadTiltPosition()
 *
 * It performs:
 *   1. Print initial voltage + degree
 *   2. Move OUTWARD for 1 second
 *   3. Print voltage + degree
 *   4. Move INWARD for 1 second
 *   5. Print voltage + degree
 *
 * This reveals:
 *   - Which relay direction increases voltage
 *   - Which relay direction decreases voltage
 *   - Whether ADC is inverted
 */

#include <stdio.h>
#include <unistd.h>
#include <time.h>

#include "mio.h"
#include "ro.h"
#include "motion.h"
#include "control_tilt.h"   /* for VoltToTilt conversion */

/* -------------------------------------------------------------------------
 * Helper: print timestamp, voltage, degree, and raw ADC
 * ------------------------------------------------------------------------- */
static void PrintStatus(const char *label)
{
    time_t now = time(NULL);
    struct tm *tm_now = localtime(&now);

    int adc = ReadTiltPosition();
    float volt = adc / 1000.0f;
    float deg  = ControlTilt_VoltToTilt(volt);

    printf("[%02d:%02d:%02d] %-12s  ADC=%5d  Volt=%.3f  Deg=%.2f\n",
           tm_now->tm_hour,
           tm_now->tm_min,
           tm_now->tm_sec,
           label,
           adc,
           volt,
           deg);
}

/* -------------------------------------------------------------------------
 * Main test
 * ------------------------------------------------------------------------- */
int main(void)
{
    printf("=== Tilt Direction Diagnostic Test ===\n");

    if (mio_init() < 0) {
        printf("ERROR: mio_init failed\n");
        return 1;
    }
    if (ro_init() < 0) {
        printf("ERROR: ro_init failed\n");
        return 1;
    }

    /* Apply calibration so VoltToTilt works */
    TiltCalibration_t cal = {
        .seat_time        = 0,
        .minimum_volts    = 0.29f,
        .maximum_volts    = 8.55f,
        .deadband         = 0.0f,
        .stop_band        = 0.0f,
        .sec_per_degree   = 0.5f,
        .max_angle        = 75.0f,
        .min_angle        = 0.0f,
        .control_time_ms  = 100
    };
    ControlTilt_ApplyCalibration(&cal);

    /* Initial reading */
    PrintStatus("Initial");
	printf("==============================");

    printf("\nRelayTilt(0,1)\n");
    RelayTilt(0, 1);   /* try direction = 0 */
    sleep(5);
    RelayTilt(0, 0);
    PrintStatus("RelayTilt(0,1)");
	printf("==============================");

    printf("\nRelayTilt(1,0)\n");
    RelayTilt(1, 0);   /* try direction = 0 */
    sleep(5);
    RelayTilt(0, 0);
    PrintStatus("RelayTilt(1,0)");
	printf("==============================");

    printf("\nRelayTilt(1,1)\n");
    RelayTilt(1, 1);   /* try direction = 0 */
    sleep(5);
    RelayTilt(0, 0);
    PrintStatus("RelayTilt(1,1)\n");
	printf("==============================");

    printf("\n=== Test Complete ===\n");
    return 0;
}

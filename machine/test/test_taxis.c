/**
 * @file test_taxis.c
 * @brief Standalone test program for T‑axis (tilt actuator) open‑loop motion.
 *
 * This program directly exercises the tilt actuator using the low‑level
 * motion layer (motion.c) without involving control_tilt.c.
 *
 * Test sequence:
 *   1. Move to Vmin (0.3 V)
 *   2. Wait 10 seconds
 *   3. Move to Vmax (8.0 V)
 *   4. Wait 10 seconds
 *   5. Move back to Vmin
 *
 * Movement is performed using a simple open‑loop algorithm:
 *   - Read current ADC
 *   - Determine direction (up/down)
 *   - Enable relays
 *   - Loop with SAMPLE_MS delay
 *   - Stop when ADC crosses target
 *
 * This program was used to validate actuator behavior and ADC scaling
 * before implementing the high‑level control layer.
 */

#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>

#include "mio.h"
#include "ro.h"
#include "motion.h"

/** @brief Convert voltage (0–10 V) to ADC counts (0–10000). */
#define VOLT_TO_ADC(v)   ((int)((v) * 1000.0))

/** @brief Minimum test voltage. */
#define VMIN   0.3f
/** @brief Maximum test voltage. */
#define VMAX   8.0f

/** @brief Sampling interval during movement (ms). */
#define SAMPLE_MS   200
/** @brief Wait time between steps (seconds). */
#define WAIT_SEC    10

/**
 * @brief Move the tilt actuator until the ADC reaches the target voltage.
 *
 * This is a simple open‑loop movement algorithm used for hardware validation.
 * It does not use calibration, pause/resume, or machine state logic.
 *
 * @param targetV Target voltage (0–10 V).
 */
void MoveToVoltage(float targetV)
{
    int target = VOLT_TO_ADC(targetV);
    printf("\n[Move] Target voltage = %.2f V (%d counts)\n", targetV, target);

    int current = ReadTiltPosition();
    if (current < 0) {
        printf("[Move] ERROR: Cannot read tilt ADC\n");
        return;
    }

    int up = (current < target) ? 1 : 0;
    printf("[Move] Direction = %s\n", up ? "UP (pull out)" : "DOWN (pull in)");

    /* Start motion */
    RelayTilt(up, 1);

    while (1) {
        usleep(SAMPLE_MS * 1000);

        current = ReadTiltPosition();
        if (current < 0) {
            printf("[Move] ERROR: ADC read failed\n");
            break;
        }

        float volts = current / 1000.0f;
        printf("[Move] ADC=%d (%.2f V)\n", current, volts);

        /* Stop when crossing target */
        if (up && current >= target)
            break;
        if (!up && current <= target)
            break;
    }

    RelayTilt(0, 0);   /* Stop motion */
    printf("[Move] Reached target %.2f V\n", targetV);
}

/**
 * @brief Main entry point for the T‑axis test program.
 *
 * Initializes HAL, then performs a sequence of open‑loop movements
 * to validate actuator behavior.
 *
 * @return 0 on success, non‑zero on HAL initialization failure.
 */
int main(void)
{
    printf("=== T‑Axis Test Program ===\n");

    /* Initialize HAL */
    if (mio_init() < 0) {
        printf("ERROR: mio_init failed\n");
        return 1;
    }
    if (ro_init() < 0) {
        printf("ERROR: ro_init failed\n");
        return 1;
    }

    printf("HAL initialized\n");
    printf("Sampling time = %d msec\n", SAMPLE_MS);

    /* ------------------------------------------------------------
     * 1. Move to Vmin
     * ------------------------------------------------------------ */
    printf("\nSTEP 1: Move to Vmin (%.2f V)\n", VMIN);
    MoveToVoltage(VMIN);

    printf("Waiting %d sec...\n", WAIT_SEC);
    sleep(WAIT_SEC);

    /* ------------------------------------------------------------
     * 2. Move from Vmin → Vmax
     * ------------------------------------------------------------ */
    printf("\nSTEP 2: Move to Vmax (%.2f V)\n", VMAX);
    MoveToVoltage(VMAX);

    printf("Waiting %d sec...\n", WAIT_SEC);
    sleep(WAIT_SEC);

    /* ------------------------------------------------------------
     * 3. Move from Vmax → Vmin
     * ------------------------------------------------------------ */
    printf("\nSTEP 3: Move back to Vmin (%.2f V)\n", VMIN);
    MoveToVoltage(VMIN);

    printf("\n=== Test Complete ===\n");
    return 0;
}

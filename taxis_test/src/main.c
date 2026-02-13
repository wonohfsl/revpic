#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>

#include "mio.h"
#include "ro.h"
#include "motion.h"

// Voltage → ADC conversion
#define VOLT_TO_ADC(v)   ((int)((v) * 1000.0))   // 0–10 V → 0–10000 counts

// Test parameters
#define VMIN   0.3
#define VMAX   8.0

#define SAMPLE_MS   200
#define WAIT_SEC    10

// Move until ADC reaches target (simple open-loop)
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

    RelayTilt(up, 1);   // start motion

    while (1) {
        usleep(SAMPLE_MS * 1000);

        current = ReadTiltPosition();
        if (current < 0) {
            printf("[Move] ERROR: ADC read failed\n");
            break;
        }

        float volts = current / 1000.0f;
        printf("[Move] ADC=%d (%.2f V)\n", current, volts);

        // Stop when we cross the target
        if (up && current >= target)
            break;
        if (!up && current <= target)
            break;
    }

    RelayTilt(0, 0);   // stop
    printf("[Move] Reached target %.2f V\n", targetV);
}

int main(void)
{
    printf("=== T‑Axis Test Program ===\n");

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

    // ------------------------------------------------------------
    // 1. Move to Vmin
    // ------------------------------------------------------------
    printf("\nSTEP 1: Move to Vmin (%.2f V)\n", VMIN);
    MoveToVoltage(VMIN);

    printf("Waiting %d sec...\n", WAIT_SEC);
    sleep(WAIT_SEC);

    // ------------------------------------------------------------
    // 2. Move from Vmin → Vmax
    // ------------------------------------------------------------
    printf("\nSTEP 2: Move to Vmax (%.2f V)\n", VMAX);
    MoveToVoltage(VMAX);

    printf("Waiting %d sec...\n", WAIT_SEC);
    sleep(WAIT_SEC);

    // ------------------------------------------------------------
    // 3. Move from Vmax → Vmin
    // ------------------------------------------------------------
    printf("\nSTEP 3: Move back to Vmin (%.2f V)\n", VMIN);
    MoveToVoltage(VMIN);

    printf("\n=== Test Complete ===\n");
    return 0;
}

// tilt.c
#include <stdio.h>
#include <stdbool.h>
#include "tilt.h"
#include "motion.h"   // ReadTiltPosition, RelayTilt

typedef enum {
    TILT_STATE_IDLE = 0,
    TILT_STATE_MOVING,
    TILT_STATE_DONE
} TiltState;

static TiltState tiltState = TILT_STATE_IDLE;
static int targetCounts = 0;   // 0–10000 (0–10 V)
static int moveUp = 1;         // 1 = pull out (up), 0 = pull in (down)

/* Tolerance around target in ADC counts (≈0.05 V) */
#define TILT_TOLERANCE_COUNTS 50

void Tilt_Start(int degree)
{
    if (degree < 0)   degree = 0;
    if (degree > 90)  degree = 90;

    // Map 0–90° → 0–10000 counts (0–10 V)
    targetCounts = (int)((degree / 90.0f) * 10000.0f);

    int current = ReadTiltPosition();
    if (current < 0) {
        printf("[Tilt] Error: failed to read tilt position\n");
        tiltState = TILT_STATE_DONE;
        return;
    }

    moveUp = (current < targetCounts) ? 1 : 0;

    printf("[Tilt] Start: degree=%d, target=%d counts, current=%d, dir=%s\n",
           degree, targetCounts, current, moveUp ? "UP (pull out)" : "DOWN (pull in)");

    // Start motion
    RelayTilt(moveUp, 1);
    tiltState = TILT_STATE_MOVING;
}

void Tilt_Stop(void)
{
    printf("[Tilt] Stop requested\n");
    RelayTilt(0, 0);          // EN=0, DIR=0
    tiltState = TILT_STATE_DONE;
}

bool Tilt_IsDone(void)
{
    return (tiltState == TILT_STATE_DONE);
}

void Tilt_Update(int paused)
{
    if (tiltState == TILT_STATE_IDLE || tiltState == TILT_STATE_DONE)
        return;

    if (paused) {
        // Pause: disable EN, keep state and direction
        RelayTilt(moveUp, 0);
        return;
    }

    int current = ReadTiltPosition();
    if (current < 0) {
        printf("[Tilt] Error: failed to read tilt position during update\n");
        Tilt_Stop();
        return;
    }

    // Check if target reached within tolerance
    if (current >= (targetCounts - TILT_TOLERANCE_COUNTS) &&
        current <= (targetCounts + TILT_TOLERANCE_COUNTS)) {

        printf("[Tilt] Target reached: current=%d, target=%d\n", current, targetCounts);
        RelayTilt(moveUp, 0);   // disable motion
        tiltState = TILT_STATE_DONE;
        return;
    }

    // Ensure motion is enabled in the correct direction
    RelayTilt(moveUp, 1);
}

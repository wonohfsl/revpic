/**
 * @file motion.c
 * @brief Implementation of mid-level motion control functions.
 *
 * These functions provide semantic machine operations by wrapping
 * low-level HAL calls (mio_get_di, mio_get_ai, ro_set_ro).
 */

#include <stdio.h>
#include <unistd.h>   // for usleep()

#include "motion.h"
#include "mio.h"
#include "ro.h"

/* -------------------------------------------------------------------------
 * Read Functions
 * ------------------------------------------------------------------------- */

int ReadEStopButton(void)
{
    return mio_get_di(DI_ESTOP);
}

int ReadHomeRotate(void)
{
    return mio_get_di(DI_PROXI_ROTATE);
}

int ReadHomeTilt(void)
{
    return mio_get_di(DI_PROXI_TILT);
}

/**
 * @brief Read the tilt position analog input (single raw ADC read).
 *
 * @return Raw ADC value (0–10000) or -1 on error.
 */
int ReadTiltADC(void)
{
    return mio_get_ai(AI_TILT_POS);
}

/**
 * @brief Read the tilt position with filtering.
 *
 * Reads the ADC three times with 1 ms spacing and returns the integer average.
 * The ADC range is 0–10000 corresponding to 0–10 V.
 *
 * NOTE:
 *   - The ADC cannot reliably measure below ~25 counts (~0.025 V).
 *   - The linear actuator's valid signal range is 0.95–9.23 V
 *     (≈950–9230 counts). Values below ~950 counts are outside the
 *     actuator's meaningful range and should be treated as "0°" or invalid.
 *
 * @return Filtered ADC value (0–10000) or -1 on error.
 */
int ReadTiltPosition(void)
{
    int a = ReadTiltADC();
    if (a < 0) return -1;

    usleep(1000);  // 1 ms

    int b = ReadTiltADC();
    if (b < 0) return -1;

    usleep(1000);  // 1 ms

    int c = ReadTiltADC();
    if (c < 0) return -1;

    return (a + b + c) / 3;
}

/* -------------------------------------------------------------------------
 * Relay Control
 * ------------------------------------------------------------------------- */

void RelayRotate(int cw, int on)
{
    if (on) {
        ro_set_ro(RO_ROTATE_DIR, cw ? 1 : 0);
        ro_set_ro(RO_ROTATE_EN, 1);
    } else {
        ro_set_ro(RO_ROTATE_EN, 0);
        ro_set_ro(RO_ROTATE_DIR, 0);
    }

    // DEBUG
    printf("RO_ROTATE_DIR (%d): %d\n", RO_ROTATE_DIR, ro_get_ro(RO_ROTATE_DIR));
    printf("RO_ROTATE_EN (%d): %d\n", RO_ROTATE_EN, ro_get_ro(RO_ROTATE_EN));
}

void RelayTilt(int up, int on)
{
    if (on) {
        ro_set_ro(RO_TILT_DIR, up ? 1 : 0);
        ro_set_ro(RO_TILT_EN, 1);
    } else {
        ro_set_ro(RO_TILT_EN, 0);
        ro_set_ro(RO_TILT_DIR, 0);
    }

    // DEBUG
    printf("RO_TILT_DIR (%d): %d\n", RO_TILT_DIR, ro_get_ro(RO_TILT_DIR));
    printf("RO_TILT_EN (%d): %d\n", RO_TILT_EN, ro_get_ro(RO_TILT_EN));
}

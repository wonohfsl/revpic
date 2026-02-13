/**
 * @file calibration_tilt.c
 * @brief Tilt-axis calibration routines.
 *
 * Provides dummy calibration logic for the T-axis. Real implementation
 * will load and store calibration data in calibration.json.
 */

#include <stdio.h>
#include "calibration_tilt.h"

// Dummy in-memory flag instead of real calibration.json
static int g_tilt_calibrated = 0;

/**
 * @brief Check whether the tilt axis is calibrated.
 *
 * Reads calibration status from in-memory flag (dummy implementation).
 * Real implementation will validate calibration.json contents.
 *
 * @return 1 if calibrated, 0 otherwise.
 */
int CalibrationTilt_Check(void)
{
    // TODO: read calibration.json and validate tilt entries
    return g_tilt_calibrated;
}

/**
 * @brief Perform tilt-axis calibration.
 *
 * Dummy implementation that simply sets the calibration flag.
 * Real implementation will measure ADC min/max, degree mapping,
 * and write results to calibration.json.
 *
 * @return 0 on success, non-zero on failure.
 */
int CalibrationTilt_Run(void)
{
    // TODO: perform real calibration and write calibration.json
    g_tilt_calibrated = 1;
    printf("Tilt calibration done (dummy)\n");
    return 0;
}

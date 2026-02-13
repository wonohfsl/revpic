/**
 * @file calibration_tilt.h
 * @brief Public API for tilt-axis calibration routines.
 *
 * Provides calibration check and calibration execution for the T-axis.
 */

#ifndef CALIBRATION_TILT_H
#define CALIBRATION_TILT_H

/**
 * @brief Check whether the tilt axis is calibrated.
 *
 * @return 1 if calibrated, 0 otherwise.
 */
int CalibrationTilt_Check(void);

/**
 * @brief Perform tilt-axis calibration.
 *
 * @return 0 on success, non-zero on failure.
 */
int CalibrationTilt_Run(void);

#endif // CALIBRATION_TILT_H

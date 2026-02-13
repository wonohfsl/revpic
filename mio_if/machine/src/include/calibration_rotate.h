/**
 * @file calibration_rotate.h
 * @brief Public API for rotation-axis calibration routines.
 *
 * Provides calibration check and calibration execution for the R-axis.
 */

#ifndef CALIBRATION_ROTATE_H
#define CALIBRATION_ROTATE_H

/**
 * @brief Check whether the rotation axis is calibrated.
 *
 * @return 1 if calibrated, 0 otherwise.
 */
int CalibrationRotate_Check(void);

/**
 * @brief Perform rotation-axis calibration.
 *
 * @return 0 on success, non-zero on failure.
 */
int CalibrationRotate_Run(void);

#endif // CALIBRATION_ROTATE_H

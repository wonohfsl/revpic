/**
 * @file control_rotate.h
 * @brief Public API for high-level R-axis (rotation) control.
 *
 * Provides a non-blocking motion engine for the rotation axis,
 * including basic calibration parameters.
 */

/* TODO:
 * - Add timing/position compensation for precision control.
 * - Add configurable rotate RPM for test tuning.
 */

#ifndef CONTROL_ROTATE_H
#define CONTROL_ROTATE_H

#include "control.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Result codes for rotation motion commands.
 */
typedef enum
{
    ROTATE_OK = 0,      /**< Motion completed successfully */
    ROTATE_RUNNING,     /**< Motion is in progress */
    ROTATE_PAUSED,      /**< Motion paused */
    ROTATE_STOPPED,     /**< Motion stopped */
    ROTATE_ERROR        /**< Motion failed */
} RotateResult_t;

/**
 * @brief Calibration structure for the rotation axis.
 */
typedef struct
{
    float rpm;               /**< Rotation speed in rpm */
    int   control_time_ms;   /**< Motion loop sampling time (ms) */
    int   timeout_margin_ms; /**< Timeout margin added to estimates (ms) */
} RotateCalibration_t;

/**
 * @brief Apply calibration values to the rotation controller.
 *
 * @param cfg Pointer to calibration structure (must not be NULL).
 */
void ControlRotate_ApplyCalibration(const RotateCalibration_t *cfg);

/**
 * @brief Check whether the rotation axis is at the HOME position.
 *
 * @return 1 if homed, 0 otherwise.
 */
int ControlRotate_CheckHome(void);

/**
 * @brief Begin a non-blocking homing sequence for the rotation axis.
 *
 * Behavior:
 *   - If HOME sensor is already active -> homed immediately.
 *   - Otherwise, rotates CW until HOME sensor becomes active.
 *   - Pause/stop requests are honored.
 *
 * @return ROTATE_OK if already homed,
 *         ROTATE_RUNNING if homing started,
 *         ROTATE_ERROR on fault.
 */
RotateResult_t ControlRotate_BeginHome(void);

/**
 * @brief Service the non-blocking homing sequence.
 *
 * @return ROTATE_RUNNING while moving,
 *         ROTATE_OK when homed,
 *         ROTATE_PAUSED or ROTATE_STOPPED on request,
 *         ROTATE_ERROR on fault.
 */
RotateResult_t ControlRotate_ServiceHome(void);

/**
 * @brief Home the rotation axis.
 *
 * @return 0 on success, non-zero on failure.
 */
int ControlRotate_Home(void);

/**
 * @brief Begin a non-blocking rotation homing sequence.
 *
 * This is a thin wrapper around ControlRotate_BeginHome().
 *
 * @return ROTATE_OK if already homed,
 *         ROTATE_RUNNING if homing started,
 *         ROTATE_ERROR on fault.
 */
RotateResult_t ControlRotate_RotateHome(void);

/**
 * @brief Begin a non-blocking rotation by degrees.
 *
 * Starts from the current position and rotates the requested degrees.
 * Uses time-based estimation at fixed speed (1 rpm).
 *
 * @param dir Rotation direction (CW or CCW).
 * @param degrees Rotation degrees (positive).
 * @return ROTATE_OK if degrees == 0,
 *         ROTATE_RUNNING if motion started,
 *         ROTATE_ERROR on invalid state.
 */
RotateResult_t ControlRotate_BeginRotate(RotateDirection_t dir,
                                         float degrees);

/**
 * @brief Begin a non-blocking full rotation until HOME is detected.
 *
 * Requires HOME to be active at start.
 *
 * The axis rotates until the HOME sensor toggles away (if needed)
 * and then returns to HOME once per full revolution.
 *
 * @param dir Rotation direction (CW or CCW).
 * @return ROTATE_RUNNING if motion started,
 *         ROTATE_ERROR on invalid state.
 */
RotateResult_t ControlRotate_BeginRotateOne(RotateDirection_t dir);

/**
 * @brief Begin a non-blocking full rotation until HOME is detected.
 *
 * Requires HOME to be active at start.
 *
 * This is a thin wrapper around ControlRotate_BeginRotateOne().
 *
 * @param dir Rotation direction (CW or CCW).
 * @return ROTATE_RUNNING if motion started,
 *         ROTATE_ERROR on invalid state.
 */
RotateResult_t ControlRotate_RotateOne(RotateDirection_t dir);

/**
 * @brief Begin a non-blocking rotation by degrees.
 *
 * This is a thin wrapper around ControlRotate_BeginRotate().
 *
 * @param dir Rotation direction (CW or CCW).
 * @param degrees Rotation degrees (positive).
 * @return ROTATE_OK if degrees == 0,
 *         ROTATE_RUNNING if motion started,
 *         ROTATE_ERROR on invalid state.
 */
RotateResult_t ControlRotate_RotateMoveToDegree(RotateDirection_t dir,
                                                float degrees);

/**
 * @brief Service the non-blocking rotation motion.
 *
 * Advances the rotation motion by one step (or time slice).
 *
 * @return ROTATE_RUNNING while motion is in progress,
 *         ROTATE_OK when target reached,
 *         ROTATE_PAUSED or ROTATE_STOPPED on request,
 *         ROTATE_ERROR on fault.
 */
RotateResult_t ControlRotate_Service(void);

/**
 * @brief Read the current rotation count.
 *
 * @param count_out Output pointer for rotation count.
 * @return 0 on success, -1 on invalid pointer.
 */
int ControlRotate_ReadPosition(int *count_out);

/**
 * @brief Read the elapsed motion time in milliseconds.
 *
 * Returns elapsed time since the current rotation motion started.
 * If no motion is active, returns 0.
 *
 * @param tick_ms_out Output pointer for elapsed time in ms.
 * @return 0 on success, -1 on invalid pointer.
 */
int ControlRotate_ReadPositionTick(int *tick_ms_out);

/**
 * @brief Pause rotation movement.
 *
 * @return 0 on success.
 */
int ControlRotate_Pause(void);

/**
 * @brief Stop rotation movement.
 *
 * @return 0 on success.
 */
int ControlRotate_Stop(void);

#ifdef __cplusplus
}
#endif

#endif /* CONTROL_ROTATE_H */

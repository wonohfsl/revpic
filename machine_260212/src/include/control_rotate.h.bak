/**
 * @file control_rotate.h
 * @brief Public API for high-level R-axis (rotation) control.
 *
 * Provides a non-blocking motion engine for the rotation axis.
 * This is a structural placeholder; real implementation will
 * interface with motion.c and hardware drivers.
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
 * @brief Check whether the rotation axis is at the HOME position.
 *
 * @return 1 if homed, 0 otherwise.
 */
int ControlRotate_CheckHome(void);

/**
 * @brief Home the rotation axis.
 *
 * @return 0 on success, non-zero on failure.
 */
int ControlRotate_Home(void);

/**
 * @brief Begin a non-blocking rotation sequence.
 *
 * Starts a rotation motion consisting of a given number of steps
 * in the specified direction. The motion is progressed by calling
 * ControlRotate_Service() periodically.
 *
 * @param dir Rotation direction (CW or CCW).
 * @param steps Number of steps to perform.
 * @return ROTATE_OK if already at target (steps == 0),
 *         ROTATE_RUNNING if motion started,
 *         ROTATE_ERROR on invalid state.
 */
RotateResult_t ControlRotate_BeginSteps(RotateDirection_t dir, int steps);

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

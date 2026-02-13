/**
 * @file control.h
 * @brief Public API for the high-level machine control state machine.
 *
 * Defines:
 *   - Machine status enumeration
 *   - Session configuration structure
 *   - Control entry points for initialization, homing, session execution,
 *     pause/resume, stop, calibration, and ESTOP handling.
 */

/* TODO:
 * - Extend session model to include rotate degrees vs steps explicitly.
 * - Add configurable tick timing for control loop testing.
 */

#ifndef CONTROL_H
#define CONTROL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief High-level machine status.
 */
typedef enum {
    MACHINE_STATUS_READY = 0,   /**< Idle and ready for a new session */
    MACHINE_STATUS_RUNNING,     /**< Session is currently executing */
    MACHINE_STATUS_PAUSED,      /**< Session is paused */
    MACHINE_STATUS_DONE,        /**< Session completed normally */
    MACHINE_STATUS_ESTOP,       /**< Emergency stop is active */
    MACHINE_STATUS_FAULT        /**< Fault condition detected */
} MachineStatus_t;

/**
 * @brief Rotation direction for the rotate axis.
 */
typedef enum {
    ROTATE_DIR_CW = 0,          /**< Clockwise rotation */
    ROTATE_DIR_CCW              /**< Counter-clockwise rotation */
} RotateDirection_t;

/**
 * @brief Configuration for a full machine session.
 *
 * A session consists of:
 *   - Moving the tilt axis to a target degree
 *   - Performing a number of rotation steps in a given direction
 */
typedef struct {
    int tilt_degree;              /**< Target tilt angle in degrees */
    RotateDirection_t rotate_dir; /**< Rotation direction */
    int rotate_num;               /**< Number of rotation steps (treated as degrees in test mode) */
} SessionConfig_t;

/**
 * @brief Initialize the control state machine.
 */
void Control_Init(void);

/**
 * @brief Periodic tick function (non-blocking orchestrator).
 *
 * Handles:
 *   - ESTOP monitoring
 *   - Non-blocking homing (tilt + rotate)
 *   - Non-blocking tilt motion
 *   - Non-blocking rotate motion
 *   - Phase transitions
 */
void Control_Tick(void);

/**
 * @brief Validate calibration for both axes.
 *
 * @return 0 if valid, -1 if calibration is missing or invalid.
 */
int Control_CheckCalibration(void);

/**
 * @brief Validate home position for both axes.
 *
 * @return 0 if both axes are homed, -1 otherwise.
 */
int Control_CheckHome(void);

/**
 * @brief Run tilt-axis calibration.
 *
 * @return 0 on success, -1 on failure.
 */
int Control_CalibrateTilt(void);

/**
 * @brief Run rotation-axis calibration.
 *
 * @return 0 on success, -1 on failure.
 */
int Control_CalibrateRotate(void);

/**
 * @brief Begin non-blocking homing of both axes.
 *
 * @return 0 on success, -1 on failure.
 */
int Control_BeginHome(void);

/**
 * @brief Blocking homing routine.
 *
 * Wraps Control_BeginHome() + Control_Tick() until complete.
 *
 * @return 0 on success, -1 on failure.
 */
int Control_Home(void);

/**
 * @brief Start a session (non-blocking).
 *
 * Begins tilt move to target degree. Rotation begins after tilt completes.
 *
 * @param cfg Pointer to session configuration.
 * @return 0 on success, -1 on failure.
 */
int Control_StartSession(const SessionConfig_t *cfg);

/**
 * @brief Pause the current session.
 *
 * @return 0 on success, -1 if not currently running.
 */
int Control_PauseSession(void);

/**
 * @brief Resume a paused session.
 *
 * @return 0 on success, -1 if not paused.
 */
int Control_ResumeSession(void);

/**
 * @brief Stop the current session.
 *
 * @return Always returns 0.
 */
int Control_StopSession(void);

/**
 * @brief Get current machine status.
 *
 * @return Current MachineStatus_t value.
 */
MachineStatus_t Control_GetStatus(void);

/**
 * @brief Notify the control system that ESTOP is active.
 */
void Control_NotifyEStopActive(void);

#ifdef __cplusplus
}
#endif

#endif /* CONTROL_H */

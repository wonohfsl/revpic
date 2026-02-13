/**
 * @file machine_state.h
 * @brief Global machine state shared by all control modules.
 */

#ifndef MACHINE_STATE_H
#define MACHINE_STATE_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Axis state machine for tilt and rotate axes.
 */
typedef enum
{
    AXIS_IDLE = 0,                 /**< Axis is idle */
    AXIS_RUNNING_TILT,             /**< Tilt axis is executing a move */
    AXIS_RUNNING_ROTATE,           /**< Rotate axis is executing a move */
    AXIS_RUNNING_TILT_CALIBRATE,   /**< Tilt axis is in calibration motion */
    AXIS_RUNNING_ROTATE_CALIBRATE  /**< Rotate axis is in calibration motion */
} AxisState_t;

/**
 * @brief Global machine state shared across main, control, and axis modules.
 */
typedef struct
{
    volatile int pause_requested;   /**< Pause command requested */
    volatile int resume_requested;  /**< Resume command requested */
    volatile int stop_requested;    /**< Stop command requested */

    volatile AxisState_t tilt_state;   /**< Tilt axis state */
    volatile AxisState_t rotate_state; /**< Rotate axis state */
} MachineState_t;

/**
 * @brief Global machine state instance.
 *
 * Defined in a single C file (e.g., machine_state.c) and referenced elsewhere.
 */
extern MachineState_t g_machine;

#ifdef __cplusplus
}
#endif

#endif /* MACHINE_STATE_H */

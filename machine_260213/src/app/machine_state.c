/**
 * @file machine_state.c
 * @brief Definition of global machine state.
 */

#include "machine_state.h"

/* Global machine state instance */
MachineState_t g_machine = {
    .pause_requested  = 0,
    .resume_requested = 0,
    .stop_requested   = 0,
    .tilt_state       = AXIS_IDLE,
    .rotate_state     = AXIS_IDLE
};

/**
 * @file control_rotate.c
 * @brief High-level control functions for the R-axis (rotation motor).
 *
 * Provides a non-blocking motion engine for the rotation axis.
 * This is a structural placeholder; real implementation will
 * interface with motion.c and hardware drivers.
 */

#include <stdio.h>
#include "control_rotate.h"

/* -------------------------------------------------------------------------
 * Internal state
 * ------------------------------------------------------------------------- */

static int g_rotate_count      = 0;  /**< Current rotation count */
static int g_rotate_is_homed   = 0;  /**< 1 if homed, 0 otherwise */

static struct
{
    int              active;        /**< 1 if a move is in progress */
    RotateDirection_t dir;          /**< Direction of motion */
    int              target_steps;  /**< Total steps to perform */
    int              done_steps;    /**< Steps completed so far */
} g_rot_motion = { 0, ROTATE_DIR_CW, 0, 0 };

/* -------------------------------------------------------------------------
 * Home / Check
 * ------------------------------------------------------------------------- */

int ControlRotate_CheckHome(void)
{
    /* Placeholder: consider homed if count is 0 */
    return (g_rotate_count == 0);
}

int ControlRotate_Home(void)
{
    g_rotate_count    = 0;
    g_rotate_is_homed = 1;
    printf("Rotate homed (dummy)\n");
    return 0;
}

/* -------------------------------------------------------------------------
 * Non-blocking motion engine
 * ------------------------------------------------------------------------- */

RotateResult_t ControlRotate_BeginSteps(RotateDirection_t dir, int steps)
{
    if (!g_rotate_is_homed) {
        return ROTATE_ERROR;
    }

    if (steps <= 0) {
        return ROTATE_OK;
    }

    g_rot_motion.active       = 1;
    g_rot_motion.dir          = dir;
    g_rot_motion.target_steps = steps;
    g_rot_motion.done_steps   = 0;

    /* Real implementation would start motor here */
    return ROTATE_RUNNING;
}

RotateResult_t ControlRotate_Service(void)
{
    if (!g_rot_motion.active) {
        return ROTATE_OK;
    }

    /* Placeholder: one step per service call */
    if (g_rot_motion.dir == ROTATE_DIR_CW) {
        g_rotate_count++;
    } else {
        g_rotate_count--;
    }

    g_rot_motion.done_steps++;

    if (g_rot_motion.done_steps >= g_rot_motion.target_steps) {
        g_rot_motion.active = 0;
        /* Real implementation would stop motor here */
        return ROTATE_OK;
    }

    return ROTATE_RUNNING;
}

/* -------------------------------------------------------------------------
 * Misc
 * ------------------------------------------------------------------------- */

int ControlRotate_ReadPosition(int *count_out)
{
    if (!count_out) return -1;
    *count_out = g_rotate_count;
    return 0;
}

int ControlRotate_Pause(void)
{
    /* Placeholder: just mark inactive; real code would stop motor */
    g_rot_motion.active = 0;
    printf("Rotate paused at count=%d (dummy)\n", g_rotate_count);
    return 0;
}

int ControlRotate_Stop(void)
{
    /* Placeholder: stop and reset motion state */
    g_rot_motion.active       = 0;
    g_rot_motion.target_steps = 0;
    g_rot_motion.done_steps   = 0;
    printf("Rotate stopped at count=%d (dummy)\n", g_rotate_count);
    return 0;
}

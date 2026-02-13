/**
 * @file control.c
 * @brief High-level machine state machine and command dispatcher.
 *
 * Orchestrates:
 *   - Calibration validation
 *   - Non-blocking homing (tilt + rotate)
 *   - Session start/stop/pause/resume
 *   - Non-blocking tilt and rotate commands
 *   - ESTOP handling
 */

#include <stdio.h>
#include <unistd.h>   /* for usleep() */
#include "control.h"
#include "control_tilt.h"
#include "control_rotate.h"
#include "calibration_tilt.h"
#include "calibration_rotate.h"
#include "machine_state.h"

/* -------------------------------------------------------------------------
 * Internal state
 * ------------------------------------------------------------------------- */

/**
 * @brief Internal control phase for session execution.
 */
typedef enum {
    CONTROL_PHASE_IDLE = 0,       /**< No active session */
    CONTROL_PHASE_HOME_TILT,      /**< Tilt homing in progress */
    CONTROL_PHASE_HOME_ROTATE,    /**< Rotate homing in progress */
    CONTROL_PHASE_TILT,           /**< Tilt axis is moving to target */
    CONTROL_PHASE_ROTATE,         /**< Rotate axis is executing steps */
    CONTROL_PHASE_DONE            /**< Session completed */
} ControlPhase_t;

static MachineStatus_t g_status          = MACHINE_STATUS_READY;
static SessionConfig_t g_session;
static int             g_estop_latched   = 0;
static ControlPhase_t  g_phase           = CONTROL_PHASE_IDLE;

/* Forward declaration */
static int CheckSession(const SessionConfig_t *cfg);

/* -------------------------------------------------------------------------
 * Initialization
 * ------------------------------------------------------------------------- */

void Control_Init(void)
{
    g_status        = MACHINE_STATUS_READY;
    g_estop_latched = 0;
    g_phase         = CONTROL_PHASE_IDLE;
}

/* -------------------------------------------------------------------------
 * Tick (non-blocking orchestrator)
 * ------------------------------------------------------------------------- */

void Control_Tick(void)
{
    /* ESTOP handling */
    if (g_estop_latched) {
        g_status = MACHINE_STATUS_ESTOP;
        return;
    }

    /* If not running, nothing to do */
    if (g_status != MACHINE_STATUS_RUNNING) {
        return;
    }

    switch (g_phase) {

    /* -------------------------------------------------------------
     * HOMING: TILT
     * ------------------------------------------------------------- */
    case CONTROL_PHASE_HOME_TILT:
    {
        TiltResult_t tr = ControlTilt_ServiceHome();

        if (tr == TILT_RUNNING) break;

        if (tr == TILT_OK) {
            /* Begin rotate homing */
            if (ControlRotate_Home() != 0) {
                g_status = MACHINE_STATUS_FAULT;
                g_phase  = CONTROL_PHASE_IDLE;
                break;
            }
            g_phase = CONTROL_PHASE_HOME_ROTATE;
            break;
        }

        if (tr == TILT_PAUSED) {
            g_status = MACHINE_STATUS_PAUSED;
            break;
        }

        g_status = MACHINE_STATUS_FAULT;
        g_phase  = CONTROL_PHASE_IDLE;
        break;
    }

    /* -------------------------------------------------------------
     * HOMING: ROTATE
     * ------------------------------------------------------------- */
    case CONTROL_PHASE_HOME_ROTATE:
    {
        /* Rotate homing is blocking/dummy for now */
        g_status = MACHINE_STATUS_READY;
        g_phase  = CONTROL_PHASE_IDLE;
        break;
    }

    /* -------------------------------------------------------------
     * SESSION: TILT
     * ------------------------------------------------------------- */
    case CONTROL_PHASE_TILT:
    {
        float actual_volt = 0.0f;
        TiltResult_t tr = ControlTilt_Service(&actual_volt);

        if (tr == TILT_RUNNING) break;

        if (tr == TILT_OK) {
            RotateResult_t rr =
                ControlRotate_BeginSteps(g_session.rotate_dir,
                                         g_session.rotate_num);

            if (rr == ROTATE_ERROR) {
                g_status = MACHINE_STATUS_FAULT;
                g_phase  = CONTROL_PHASE_IDLE;
            } else if (rr == ROTATE_OK) {
                g_status = MACHINE_STATUS_DONE;
                g_phase  = CONTROL_PHASE_DONE;
            } else {
                g_phase = CONTROL_PHASE_ROTATE;
            }
            break;
        }

        if (tr == TILT_PAUSED) {
            g_status = MACHINE_STATUS_PAUSED;
            break;
        }

        g_status = MACHINE_STATUS_FAULT;
        g_phase  = CONTROL_PHASE_IDLE;
        break;
    }

    /* -------------------------------------------------------------
     * SESSION: ROTATE
     * ------------------------------------------------------------- */
    case CONTROL_PHASE_ROTATE:
    {
        RotateResult_t rr = ControlRotate_Service();

        if (rr == ROTATE_RUNNING) break;

        if (rr == ROTATE_OK) {
            g_status = MACHINE_STATUS_DONE;
            g_phase  = CONTROL_PHASE_DONE;
            break;
        }

        if (rr == ROTATE_PAUSED) {
            g_status = MACHINE_STATUS_PAUSED;
            break;
        }

        g_status = MACHINE_STATUS_FAULT;
        g_phase  = CONTROL_PHASE_IDLE;
        break;
    }

    case CONTROL_PHASE_DONE:
    case CONTROL_PHASE_IDLE:
    default:
        break;
    }
}

/* -------------------------------------------------------------------------
 * Calibration and Home Checks
 * ------------------------------------------------------------------------- */

int Control_CheckCalibration(void)
{
    int tilt_ok = CalibrationTilt_Check();
    int rot_ok  = CalibrationRotate_Check();

    if (!tilt_ok || !rot_ok) {
        g_status = MACHINE_STATUS_FAULT;
        return -1;
    }
    return 0;
}

int Control_CheckHome(void)
{
    int tilt_ok = ControlTilt_CheckHome();
    int rot_ok  = ControlRotate_CheckHome();

    if (!tilt_ok || !rot_ok) {
        g_status = MACHINE_STATUS_FAULT;
        return -1;
    }
    return 0;
}

/* -------------------------------------------------------------------------
 * Calibration Routines
 * ------------------------------------------------------------------------- */

int Control_CalibrateTilt(void)
{
    if (CalibrationTilt_Run() != 0) {
        g_status = MACHINE_STATUS_FAULT;
        return -1;
    }
    return 0;
}

int Control_CalibrateRotate(void)
{
    if (CalibrationRotate_Run() != 0) {
        g_status = MACHINE_STATUS_FAULT;
        return -1;
    }
    return 0;
}

/* -------------------------------------------------------------------------
 * Homing (non-blocking)
 * ------------------------------------------------------------------------- */

int Control_BeginHome(void)
{
    if (g_status != MACHINE_STATUS_READY &&
        g_status != MACHINE_STATUS_DONE)
    {
        g_status = MACHINE_STATUS_FAULT;
        return -1;
    }

    TiltResult_t tr = ControlTilt_BeginHome();

    if (tr == TILT_ERROR) {
        g_status = MACHINE_STATUS_FAULT;
        return -1;
    }

    if (tr == TILT_OK) {
        /* Tilt already homed → go to rotate homing */
        if (ControlRotate_Home() != 0) {
            g_status = MACHINE_STATUS_FAULT;
            return -1;
        }
        g_status = MACHINE_STATUS_READY;
        g_phase  = CONTROL_PHASE_IDLE;
        return 0;
    }

    g_status = MACHINE_STATUS_RUNNING;
    g_phase  = CONTROL_PHASE_HOME_TILT;
    return 0;
}

int Control_Home(void)
{
    if (Control_BeginHome() != 0) return -1;

    while (g_status == MACHINE_STATUS_RUNNING) {
        Control_Tick();
        usleep(1000);
    }

    return (g_status == MACHINE_STATUS_READY) ? 0 : -1;
}

/* -------------------------------------------------------------------------
 * Session Start (non-blocking)
 * ------------------------------------------------------------------------- */

int Control_StartSession(const SessionConfig_t *cfg)
{
    if (g_status != MACHINE_STATUS_READY &&
        g_status != MACHINE_STATUS_DONE)
    {
        g_status = MACHINE_STATUS_FAULT;
        return -1;
    }

    if (CheckSession(cfg) != 0) {
        g_status = MACHINE_STATUS_FAULT;
        return -1;
    }

    if (Control_CheckCalibration() != 0) return -1;
    if (Control_CheckHome() != 0)        return -1;

    g_session = *cfg;

    TiltResult_t tr =
        ControlTilt_BeginMoveToDegree(cfg->tilt_degree);

    if (tr == TILT_ERROR || tr == TILT_STOPPED) {
        g_status = MACHINE_STATUS_FAULT;
        g_phase  = CONTROL_PHASE_IDLE;
        return -1;
    }

    if (tr == TILT_OK) {
        RotateResult_t rr =
            ControlRotate_BeginSteps(cfg->rotate_dir,
                                     cfg->rotate_num);

        if (rr == ROTATE_ERROR) {
            g_status = MACHINE_STATUS_FAULT;
            g_phase  = CONTROL_PHASE_IDLE;
            return -1;
        }

        if (rr == ROTATE_OK) {
            g_status = MACHINE_STATUS_DONE;
            g_phase  = CONTROL_PHASE_DONE;
            return 0;
        }

        g_status = MACHINE_STATUS_RUNNING;
        g_phase  = CONTROL_PHASE_ROTATE;
        return 0;
    }

    g_status = MACHINE_STATUS_RUNNING;
    g_phase  = CONTROL_PHASE_TILT;
    return 0;
}

/* -------------------------------------------------------------------------
 * Pause / Resume / Stop
 * ------------------------------------------------------------------------- */

int Control_PauseSession(void)
{
    if (g_status != MACHINE_STATUS_RUNNING) {
        g_status = MACHINE_STATUS_FAULT;
        return -1;
    }

    ControlTilt_Pause();
    ControlRotate_Pause();

    g_status = MACHINE_STATUS_PAUSED;
    return 0;
}

int Control_ResumeSession(void)
{
    if (g_status != MACHINE_STATUS_PAUSED) {
        g_status = MACHINE_STATUS_FAULT;
        return -1;
    }

    if (g_phase == CONTROL_PHASE_TILT) {
        TiltResult_t tr =
            ControlTilt_BeginMoveToDegree(g_session.tilt_degree);
        if (tr == TILT_ERROR) {
            g_status = MACHINE_STATUS_FAULT;
            g_phase  = CONTROL_PHASE_IDLE;
            return -1;
        }
    }
    else if (g_phase == CONTROL_PHASE_ROTATE) {
        RotateResult_t rr =
            ControlRotate_BeginSteps(g_session.rotate_dir,
                                     g_session.rotate_num);
        if (rr == ROTATE_ERROR) {
            g_status = MACHINE_STATUS_FAULT;
            g_phase  = CONTROL_PHASE_IDLE;
            return -1;
        }
    }

    g_status = MACHINE_STATUS_RUNNING;
    return 0;
}

int Control_StopSession(void)
{
    ControlTilt_Pause();
    ControlRotate_Stop();
    g_status = MACHINE_STATUS_FAULT;
    g_phase  = CONTROL_PHASE_IDLE;
    return 0;
}

/* -------------------------------------------------------------------------
 * Status / ESTOP
 * ------------------------------------------------------------------------- */

MachineStatus_t Control_GetStatus(void)
{
    return g_status;
}

void Control_NotifyEStopActive(void)
{
    g_estop_latched = 1;
}

/* -------------------------------------------------------------------------
 * Session Validation
 * ------------------------------------------------------------------------- */

static int CheckSession(const SessionConfig_t *cfg)
{
    if (!cfg) return -1;

    if (cfg->rotate_num < 0) return -1;

    if (cfg->tilt_degree < 0 || cfg->tilt_degree > 90) {
        return -1;
    }

    return 0;
}

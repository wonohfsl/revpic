/**
 * @file control_rotate.c
 * @brief High-level control functions for the R-axis (rotation motor).
 *
 * Implements:
 *   - Homing (non-blocking)
 *   - Time-based rotate by degree (non-blocking)
 *   - Full-rotation until HOME (non-blocking)
 */

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include "control_rotate.h"
#include "machine_state.h"
#include "motion.h"
#include "mio.h"

/* TODO:
 * - Add timing/position compensation for precision control.
 * - Add a configurable RPM parameter for test tuning.
 */

/* -------------------------------------------------------------------------
 * Internal calibration and state
 * ------------------------------------------------------------------------- */

static struct
{
    float rpm;
    int   control_time_ms;
    int   timeout_margin_ms;
} g_cal = {
    .rpm              = 1.0f,
    .control_time_ms  = 100,
    .timeout_margin_ms = 5000
};

/**
 * @brief Apply calibration values to the rotation controller.
 *
 * @param cfg Pointer to calibration structure (must not be NULL).
 */
void ControlRotate_ApplyCalibration(const RotateCalibration_t *cfg)
{
    if (!cfg) return;

    g_cal.rpm              = cfg->rpm;
    g_cal.control_time_ms  = cfg->control_time_ms;
    g_cal.timeout_margin_ms = cfg->timeout_margin_ms;
}

/**
 * @brief Rotate motion mode.
 */
typedef enum
{
    ROTATE_MODE_NONE = 0,   /**< No motion */
    ROTATE_MODE_DEGREE,     /**< Rotate for estimated degrees */
    ROTATE_MODE_ONE         /**< Rotate one full round until HOME */
} RotateMode_t;

/**
 * @brief Internal state for RotateOne().
 */
typedef enum
{
    ROTATE_ONE_WAIT_CLEAR = 0,  /**< Wait until HOME clears */
    ROTATE_ONE_WAIT_HOME       /**< Wait until HOME is detected */
} RotateOneState_t;

static int   g_rotate_is_homed = 0;
static float g_rotate_est_deg  = 0.0f;

/**
 * @brief Internal homing state.
 */
static struct
{
    int      active;        /**< 1 if homing is in progress */
    uint64_t start_ms;      /**< Homing start time */
    uint64_t last_tick_ms;  /**< Last control tick time */
    uint64_t timeout_ms;    /**< Homing timeout */
    int      last_raw;      /**< Last raw sensor read */
    int      last_home;     /**< Last interpreted home read */
} g_home = { 0, 0, 0, 0, -1, -1 };

/**
 * @brief Internal motion state.
 */
static struct
{
    int              active;         /**< 1 if move is in progress */
    RotateMode_t     mode;           /**< Motion mode */
    RotateDirection_t dir;           /**< Rotation direction */
    float            target_degrees; /**< Target degrees for degree mode */
    float            start_est_deg;  /**< Estimated degrees at motion start */
    uint64_t         start_ms;       /**< Motion start time */
    uint64_t         duration_ms;    /**< Target duration for degree mode */
    uint64_t         timeout_ms;     /**< Timeout for rotate-one */
    uint64_t         last_tick_ms;   /**< Last control tick time */
    RotateOneState_t one_state;      /**< Rotate-one sub-state */
    int              last_raw;       /**< Last raw sensor read */
    int              last_home;      /**< Last interpreted home read */
} g_motion = { 0, ROTATE_MODE_NONE, ROTATE_DIR_CW, 0.0f, 0.0f, 0, 0, 0, 0,
               ROTATE_ONE_WAIT_CLEAR, -1, -1 };

/* -------------------------------------------------------------------------
 * Time helpers
 * ------------------------------------------------------------------------- */

/**
 * @brief Get a monotonic timestamp in milliseconds.
 *
 * @return Monotonic time in milliseconds.
 */
static uint64_t ControlRotate_NowMs(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000ULL + (uint64_t)ts.tv_nsec / 1000000ULL;
}

/**
 * @brief Check whether a new control tick should run.
 *
 * @param last_tick_ms Pointer to the last tick timestamp.
 * @param interval_ms  Control interval in milliseconds.
 * @return 1 if a tick should run, 0 otherwise.
 */
static int ControlRotate_ShouldTick(uint64_t *last_tick_ms, int interval_ms)
{
    uint64_t now = ControlRotate_NowMs();

    if (!last_tick_ms) return 1;

    if (interval_ms <= 0) {
        *last_tick_ms = now;
        return 1;
    }

    if (*last_tick_ms == 0 || now - *last_tick_ms >= (uint64_t)interval_ms) {
        *last_tick_ms = now;
        return 1;
    }

    return 0;
}

/* -------------------------------------------------------------------------
 * Motion helpers
 * ------------------------------------------------------------------------- */

/**
 * @brief Convert degrees to duration (ms) using fixed 1 rpm.
 *
 * @param degrees Rotation degrees (positive).
 * @return Estimated duration in milliseconds.
 */
static uint64_t ControlRotate_DegreesToDurationMs(float degrees)
{
    float deg = fabsf(degrees);
    float deg_per_sec = g_cal.rpm * 6.0f; /* 360 deg/min = 6 deg/sec */

    if (deg <= 0.0f || deg_per_sec <= 0.0f) return 0U;

    uint64_t ms = (uint64_t)((deg / deg_per_sec) * 1000.0f);
    if (ms < 200U) ms = 200U;
    return ms;
}

/**
 * @brief Compute a timeout for a full rotation at fixed speed.
 *
 * @return Timeout in milliseconds.
 */
static uint64_t ControlRotate_FullRotationTimeoutMs(void)
{
    float deg_per_sec = g_cal.rpm * 6.0f;

    if (deg_per_sec <= 0.0f) return 70000U;

    uint64_t ms = (uint64_t)((360.0f / deg_per_sec) * 1000.0f);
    if (ms < 10000U) ms = 10000U;
    return ms + (uint64_t)g_cal.timeout_margin_ms;
}

/**
 * @brief Compute the nominal duration for one full rotation (ms).
 *
 * @return Duration in milliseconds.
 */
static uint64_t ControlRotate_FullRotationDurationMs(void)
{
    float deg_per_sec = g_cal.rpm * 6.0f;

    if (deg_per_sec <= 0.0f) return 0U;

    return (uint64_t)((360.0f / deg_per_sec) * 1000.0f);
}

/**
 * @brief Read the raw rotate home sensor (DI) value.
 *
 * @return Raw DI value (1 = not touched, 0 = touched) or -1 on error.
 */
static int ControlRotate_ReadHomeRaw(void)
{
    return mio_get_di(DI_PROXI_ROTATE);
}

/**
 * @brief Log raw and interpreted home sensor readings.
 *
 * @param tag Context string.
 * @param raw Raw DI value.
 * @param home Interpreted HOME value (1 = home detected).
 */
static void ControlRotate_LogHomeSensor(const char *tag, int raw, int home)
{
    if (!tag) tag = "sensor";

    printf("Rotate %s: home_raw=%d (1=not touched, 0=touched), home=%d\n",
           tag, raw, home);
}

/* -------------------------------------------------------------------------
 * Home / Check
 * ------------------------------------------------------------------------- */

/**
 * @brief Check whether the rotation axis is at HOME position.
 *
 * @return 1 if homed, 0 otherwise.
 */
int ControlRotate_CheckHome(void)
{
    int home = ReadHomeRotate();

    if (home < 0) return 0;

    if (home) {
        g_rotate_is_homed = 1;
        g_rotate_est_deg  = 0.0f;
    }

    return home ? 1 : 0;
}

/**
 * @brief Begin a non-blocking homing sequence.
 *
 * @return ROTATE_OK if already homed,
 *         ROTATE_RUNNING if homing started,
 *         ROTATE_ERROR on fault.
 */
RotateResult_t ControlRotate_BeginHome(void)
{
    int home = ReadHomeRotate();

    if (home < 0) return ROTATE_ERROR;

    if (home) {
        g_rotate_is_homed = 1;
        g_rotate_est_deg  = 0.0f;
        g_home.active     = 0;
        g_machine.rotate_state = AXIS_IDLE;
        return ROTATE_OK;
    }

    printf("RotateHome: touch metal to simulate HOME. ");
    printf("Expect home_raw=1 when not touched, 0 when touched.\n");

    g_home.active     = 1;
    g_machine.rotate_state = AXIS_RUNNING_ROTATE;
    g_rotate_is_homed = 0;

    uint64_t now = ControlRotate_NowMs();
    g_home.start_ms     = now;
    g_home.last_tick_ms = 0;
    g_home.timeout_ms   = ControlRotate_FullRotationTimeoutMs();

    g_home.last_raw  = ControlRotate_ReadHomeRaw();
    g_home.last_home = home;

    if (g_home.last_raw >= 0) {
        ControlRotate_LogHomeSensor("home-start", g_home.last_raw, home);
    }

    RelayRotate(1, 1); /* Always rotate CW for homing */

    return ROTATE_RUNNING;
}

/**
 * @brief Service the non-blocking homing sequence.
 *
 * @return ROTATE_RUNNING while moving,
 *         ROTATE_OK when homed,
 *         ROTATE_PAUSED or ROTATE_STOPPED on request,
 *         ROTATE_ERROR on fault.
 */
RotateResult_t ControlRotate_ServiceHome(void)
{
    if (!g_home.active) {
        return ROTATE_OK;
    }

    if (g_machine.pause_requested) {
        RelayRotate(0, 0);
        g_home.active = 0;
        g_machine.rotate_state = AXIS_IDLE;
        return ROTATE_PAUSED;
    }

    if (g_machine.stop_requested) {
        RelayRotate(0, 0);
        g_home.active = 0;
        g_machine.rotate_state = AXIS_IDLE;
        return ROTATE_STOPPED;
    }

    if (!ControlRotate_ShouldTick(&g_home.last_tick_ms, g_cal.control_time_ms)) {
        return ROTATE_RUNNING;
    }

    int raw  = ControlRotate_ReadHomeRaw();
    int home = ReadHomeRotate();

    if (raw < 0 || home < 0) {
        RelayRotate(0, 0);
        g_home.active = 0;
        g_machine.rotate_state = AXIS_IDLE;
        return ROTATE_ERROR;
    }

    if (raw != g_home.last_raw || home != g_home.last_home) {
        ControlRotate_LogHomeSensor("home-change", raw, home);
        g_home.last_raw  = raw;
        g_home.last_home = home;
    }

    if (home) {
        RelayRotate(0, 0);
        g_home.active = 0;
        g_rotate_is_homed = 1;
        g_rotate_est_deg  = 0.0f;
        g_machine.rotate_state = AXIS_IDLE;
        return ROTATE_OK;
    }

    uint64_t now = ControlRotate_NowMs();
    if (now - g_home.start_ms > g_home.timeout_ms) {
        RelayRotate(0, 0);
        g_home.active = 0;
        g_machine.rotate_state = AXIS_IDLE;
        return ROTATE_ERROR;
    }

    return ROTATE_RUNNING;
}

/**
 * @brief Blocking homing routine.
 *
 * @return 0 on success, non-zero on failure.
 */
int ControlRotate_Home(void)
{
    RotateResult_t r = ControlRotate_BeginHome();
    if (r == ROTATE_OK) return 0;
    if (r == ROTATE_ERROR) return -1;

    while (1) {
        r = ControlRotate_ServiceHome();
        if (r == ROTATE_RUNNING) continue;
        if (r == ROTATE_OK) return 0;
        return -1;
    }
}

/**
 * @brief Begin a non-blocking rotation homing sequence.
 *
 * This is a thin wrapper around ControlRotate_BeginHome().
 *
 * @return ROTATE_OK if already homed,
 *         ROTATE_RUNNING if homing started,
 *         ROTATE_ERROR on fault.
 */
RotateResult_t ControlRotate_RotateHome(void)
{
    return ControlRotate_BeginHome();
}

/* -------------------------------------------------------------------------
 * Non-blocking motion engine
 * ------------------------------------------------------------------------- */

/**
 * @brief Begin a non-blocking rotation by degrees.
 *
 * Starts from the current position and rotates the requested degrees.
 *
 * @param dir Rotation direction.
 * @param degrees Rotation degrees (positive).
 * @return ROTATE_OK if degrees == 0,
 *         ROTATE_RUNNING if motion started,
 *         ROTATE_ERROR on invalid state.
 */
RotateResult_t ControlRotate_BeginRotate(RotateDirection_t dir, float degrees)
{
    uint64_t duration = ControlRotate_DegreesToDurationMs(degrees);
    if (duration == 0U) {
        return ROTATE_OK;
    }

    g_motion.active         = 1;
    g_motion.mode           = ROTATE_MODE_DEGREE;
    g_motion.dir            = dir;
    g_motion.target_degrees = fabsf(degrees);
    g_motion.start_est_deg  = g_rotate_est_deg;
    g_motion.start_ms       = ControlRotate_NowMs();
    g_motion.duration_ms    = duration;
    g_motion.timeout_ms     = duration + (uint64_t)g_cal.timeout_margin_ms;
    g_motion.last_tick_ms   = 0;
    g_machine.rotate_state  = AXIS_RUNNING_ROTATE;
    g_rotate_is_homed       = 0;

    RelayRotate(dir == ROTATE_DIR_CW, 1);

    return ROTATE_RUNNING;
}

/**
 * @brief Begin a non-blocking full rotation until HOME is detected.
 *
 * Requires HOME to be active at start.
 *
 * @param dir Rotation direction.
 * @return ROTATE_RUNNING if motion started,
 *         ROTATE_ERROR on invalid state.
 */
RotateResult_t ControlRotate_BeginRotateOne(RotateDirection_t dir)
{
    int home = ReadHomeRotate();
    if (home < 0) return ROTATE_ERROR;
    if (!home) return ROTATE_ERROR;

    g_rotate_is_homed = 1;

    printf("RotateOne: touch metal to simulate HOME. ");
    printf("Expect home_raw=1 when not touched, 0 when touched.\n");

    g_motion.active         = 1;
    g_motion.mode           = ROTATE_MODE_ONE;
    g_motion.dir            = dir;
    g_motion.target_degrees = 360.0f;
    g_motion.start_est_deg  = g_rotate_est_deg;
    g_motion.start_ms       = ControlRotate_NowMs();
    g_motion.duration_ms    = ControlRotate_FullRotationDurationMs();
    g_motion.timeout_ms     = ControlRotate_FullRotationTimeoutMs();
    g_motion.last_tick_ms   = 0;
    g_motion.one_state      = home ? ROTATE_ONE_WAIT_CLEAR
                                   : ROTATE_ONE_WAIT_HOME;
    g_motion.last_raw       = ControlRotate_ReadHomeRaw();
    g_motion.last_home      = home;
    g_machine.rotate_state  = AXIS_RUNNING_ROTATE;
    g_rotate_is_homed       = 0;

    if (g_motion.last_raw >= 0) {
        ControlRotate_LogHomeSensor("one-start", g_motion.last_raw, home);
    }

    RelayRotate(dir == ROTATE_DIR_CW, 1);

    return ROTATE_RUNNING;
}

/**
 * @brief Begin a non-blocking full rotation until HOME is detected.
 *
 * This is a thin wrapper around ControlRotate_BeginRotateOne().
 *
 * @param dir Rotation direction.
 * @return ROTATE_RUNNING if motion started,
 *         ROTATE_ERROR on invalid state.
 */
RotateResult_t ControlRotate_RotateOne(RotateDirection_t dir)
{
    return ControlRotate_BeginRotateOne(dir);
}

/**
 * @brief Begin a non-blocking rotation by degrees.
 *
 * This is a thin wrapper around ControlRotate_BeginRotate().
 *
 * @param dir Rotation direction.
 * @param degrees Rotation degrees (positive).
 * @return ROTATE_OK if degrees == 0,
 *         ROTATE_RUNNING if motion started,
 *         ROTATE_ERROR on invalid state.
 */
RotateResult_t ControlRotate_RotateMoveToDegree(RotateDirection_t dir,
                                                float degrees)
{
    return ControlRotate_BeginRotate(dir, degrees);
}

/**
 * @brief Service the non-blocking rotation motion.
 *
 * @return ROTATE_RUNNING while motion is in progress,
 *         ROTATE_OK when target reached,
 *         ROTATE_PAUSED or ROTATE_STOPPED on request,
 *         ROTATE_ERROR on fault.
 */
RotateResult_t ControlRotate_Service(void)
{
    if (!g_motion.active) {
        return ROTATE_OK;
    }

    if (g_machine.pause_requested) {
        RelayRotate(0, 0);
        g_motion.active = 0;
        g_machine.rotate_state = AXIS_IDLE;
        return ROTATE_PAUSED;
    }

    if (g_machine.stop_requested) {
        RelayRotate(0, 0);
        g_motion.active = 0;
        g_machine.rotate_state = AXIS_IDLE;
        return ROTATE_STOPPED;
    }

    if (!ControlRotate_ShouldTick(&g_motion.last_tick_ms, g_cal.control_time_ms)) {
        return ROTATE_RUNNING;
    }

    uint64_t now = ControlRotate_NowMs();

    if (g_motion.mode == ROTATE_MODE_DEGREE) {
        float elapsed_ms = (float)(now - g_motion.start_ms);
        float fraction = (g_motion.duration_ms > 0)
                         ? (elapsed_ms / (float)g_motion.duration_ms)
                         : 1.0f;

        if (fraction < 0.0f) fraction = 0.0f;
        if (fraction > 1.0f) fraction = 1.0f;

        float progress_deg = g_motion.target_degrees * fraction;
        if (g_motion.dir == ROTATE_DIR_CW) {
            g_rotate_est_deg = g_motion.start_est_deg + progress_deg;
        } else {
            g_rotate_est_deg = g_motion.start_est_deg - progress_deg;
        }

        if (now - g_motion.start_ms >= g_motion.duration_ms) {
            RelayRotate(0, 0);
            g_motion.active = 0;
            g_machine.rotate_state = AXIS_IDLE;

            if (g_motion.dir == ROTATE_DIR_CW) {
                g_rotate_est_deg = g_motion.start_est_deg + g_motion.target_degrees;
            } else {
                g_rotate_est_deg = g_motion.start_est_deg - g_motion.target_degrees;
            }

            return ROTATE_OK;
        }

        if (now - g_motion.start_ms > g_motion.timeout_ms) {
            RelayRotate(0, 0);
            g_motion.active = 0;
            g_machine.rotate_state = AXIS_IDLE;
            return ROTATE_ERROR;
        }

        return ROTATE_RUNNING;
    }

    if (g_motion.mode == ROTATE_MODE_ONE) {
        float elapsed_ms = (float)(now - g_motion.start_ms);
        float fraction = (g_motion.duration_ms > 0)
                         ? (elapsed_ms / (float)g_motion.duration_ms)
                         : 0.0f;

        if (fraction < 0.0f) fraction = 0.0f;
        if (fraction > 1.0f) fraction = 1.0f;

        float progress_deg = 360.0f * fraction;
        if (g_motion.dir == ROTATE_DIR_CW) {
            g_rotate_est_deg = g_motion.start_est_deg + progress_deg;
        } else {
            g_rotate_est_deg = g_motion.start_est_deg - progress_deg;
        }

        int raw  = ControlRotate_ReadHomeRaw();
        int home = ReadHomeRotate();

        if (raw < 0 || home < 0) {
            RelayRotate(0, 0);
            g_motion.active = 0;
            g_machine.rotate_state = AXIS_IDLE;
            return ROTATE_ERROR;
        }

        if (raw != g_motion.last_raw || home != g_motion.last_home) {
            ControlRotate_LogHomeSensor("one-change", raw, home);
            g_motion.last_raw  = raw;
            g_motion.last_home = home;
        }

        if (g_motion.one_state == ROTATE_ONE_WAIT_CLEAR) {
            if (!home) {
                g_motion.one_state = ROTATE_ONE_WAIT_HOME;
            }
        } else {
            if (home) {
                RelayRotate(0, 0);
                g_motion.active = 0;
                g_machine.rotate_state = AXIS_IDLE;
                g_rotate_is_homed = 1;
                g_rotate_est_deg  = 0.0f;
                return ROTATE_OK;
            }
        }

        if (now - g_motion.start_ms > g_motion.timeout_ms) {
            RelayRotate(0, 0);
            g_motion.active = 0;
            g_machine.rotate_state = AXIS_IDLE;
            return ROTATE_ERROR;
        }

        return ROTATE_RUNNING;
    }

    RelayRotate(0, 0);
    g_motion.active = 0;
    g_machine.rotate_state = AXIS_IDLE;
    return ROTATE_ERROR;
}

/* -------------------------------------------------------------------------
 * Misc
 * ------------------------------------------------------------------------- */

/**
 * @brief Read the current estimated rotation position (degrees).
 *
 * @param count_out Output pointer for position (degrees).
 * @return 0 on success, -1 on invalid pointer.
 */
int ControlRotate_ReadPosition(int *count_out)
{
    if (!count_out) return -1;
    *count_out = (int)lroundf(g_rotate_est_deg);
    return 0;
}

/**
 * @brief Read the elapsed motion time in milliseconds.
 *
 * @param tick_ms_out Output pointer for elapsed time in ms.
 * @return 0 on success, -1 on invalid pointer.
 */
int ControlRotate_ReadPositionTick(int *tick_ms_out)
{
    if (!tick_ms_out) return -1;

    if (!g_motion.active || g_motion.start_ms == 0) {
        *tick_ms_out = 0;
        return 0;
    }

    uint64_t now = ControlRotate_NowMs();
    *tick_ms_out = (int)(now - g_motion.start_ms);
    return 0;
}

/**
 * @brief Pause rotation movement immediately.
 *
 * @return 0 on success.
 */
int ControlRotate_Pause(void)
{
    RelayRotate(0, 0);
    g_motion.active = 0;
    g_machine.rotate_state = AXIS_IDLE;
    return 0;
}

/**
 * @brief Stop rotation movement immediately.
 *
 * @return 0 on success.
 */
int ControlRotate_Stop(void)
{
    RelayRotate(0, 0);
    g_motion.active = 0;
    g_motion.mode   = ROTATE_MODE_NONE;
    g_motion.target_degrees = 0.0f;
    g_machine.rotate_state = AXIS_IDLE;
    return 0;
}

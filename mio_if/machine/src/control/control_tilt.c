/**
 * @file control_tilt.c
 * @brief High-level control functions for the tilt actuator.
 *
 * Implements:
 *   - Calibration
 *   - Degree/voltage conversion
 *   - Homing (non-blocking)
 *   - Non-blocking motion engine with pause/stop handling
 *   - Stop-band compensation (in/out, in volts)
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include "control_tilt.h"
#include "motion.h"
#include "machine_state.h"

/* TODO:
 * - Apply home-offset compensation after prox triggers.
 * - Enforce minimum-voltage safety near mechanical stop.
 * - Add timeout protection for homing and moves.
 * - Add jam/no-movement detection.
 * - Add optional heartbeat/watchdog integration.
 */

/* -------------------------------------------------------------------------
 * Internal calibration and state
 * ------------------------------------------------------------------------- */

static struct
{
    int   seat_time_ms;
    float min_volts;
    float max_volts;
    float deadband;
    float stop_band_in;
    float stop_band_out;
    float sec_per_degree;
    float max_angle;
    float min_angle;
    int   control_time_ms;
} g_cal = {
    .seat_time_ms    = 200,
    .min_volts       = 0.29f,
    .max_volts       = 8.55f,
    .deadband        = 0.0f,
    .stop_band_in    = 0.2f,
    .stop_band_out   = 0.2f,
    .sec_per_degree  = 0.5f,
    .max_angle       = 75.0f,
    .min_angle       = 0.0f,
    .control_time_ms = 100
};

static int   g_tilt_is_homed = 0;
static float g_last_degree   = 0.0f;

/* -------------------------------------------------------------------------
 * Internal motion state
 * ------------------------------------------------------------------------- */

static struct
{
    int      active;
    int      target_adc;
    int      direction_up;
    int      last_adc;
    uint64_t start_ms;
    uint64_t last_progress_ms;
    uint64_t last_tick_ms;
    uint64_t timeout_ms;
} g_motion = { 0, 0, 0, 0, 0, 0, 0, 0 };

/**
 * @brief Internal homing state.
 */
static struct
{
    int      active;            /**< 1 if homing is in progress */
    int      last_adc;          /**< Last observed ADC value */
    uint64_t start_ms;          /**< Homing start time */
    uint64_t last_progress_ms;  /**< Last time motion progressed */
    uint64_t last_tick_ms;      /**< Last tick time */
    uint64_t timeout_ms;        /**< Homing timeout */
} g_home = { 0, 0, 0, 0, 0, 0 };

static const uint64_t k_timeout_margin_ms     = 2000U;
static const uint64_t k_stall_timeout_ms      = 2000U;
static const int      k_progress_adc_threshold = 5;

/**
 * @brief Get a monotonic timestamp in milliseconds.
 *
 * @return Monotonic time in milliseconds.
 */
static uint64_t ControlTilt_NowMs(void)
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
static int ControlTilt_ShouldTick(uint64_t *last_tick_ms, int interval_ms)
{
    uint64_t now = ControlTilt_NowMs();

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
 * Calibration
 * ------------------------------------------------------------------------- */

/**
 * @brief Apply calibration values to the tilt controller.
 *
 * @param cfg Pointer to calibration structure (must not be NULL).
 */
void ControlTilt_ApplyCalibration(const TiltCalibration_t *cfg)
{
    if (!cfg) return;

    g_cal.seat_time_ms    = cfg->seat_time_ms;
    g_cal.min_volts       = cfg->minimum_volts;
    g_cal.max_volts       = cfg->maximum_volts;
    g_cal.deadband        = cfg->deadband;
    g_cal.stop_band_in    = cfg->stop_band_in;
    g_cal.stop_band_out   = cfg->stop_band_out;
    g_cal.sec_per_degree  = cfg->sec_per_degree;
    g_cal.max_angle       = cfg->max_angle;
    g_cal.min_angle       = cfg->min_angle;
    g_cal.control_time_ms = cfg->control_time_ms;
}

/* -------------------------------------------------------------------------
 * Conversion helpers
 * ------------------------------------------------------------------------- */

/**
 * @brief Convert tilt degrees to sensor voltage.
 *
 * @param degree Tilt angle in degrees.
 * @return Voltage corresponding to the given angle.
 */
float ControlTilt_TiltToVolt(float degree)
{
    float span_deg  = g_cal.max_angle - g_cal.min_angle;
    float span_volt = g_cal.max_volts - g_cal.min_volts;

    if (span_deg <= 0.0f) return g_cal.min_volts;

    return g_cal.min_volts +
           (degree - g_cal.min_angle) * (span_volt / span_deg);
}

/**
 * @brief Convert sensor voltage to tilt degrees.
 *
 * @param volts Sensor voltage.
 * @return Tilt angle in degrees.
 */
float ControlTilt_VoltToTilt(float volts)
{
    float span_deg  = g_cal.max_angle - g_cal.min_angle;
    float span_volt = g_cal.max_volts - g_cal.min_volts;

    if (span_volt <= 0.0f) return g_cal.min_angle;

    return g_cal.min_angle +
           (volts - g_cal.min_volts) * (span_deg / span_volt);
}

/**
 * @brief Compute a conservative timeout for a tilt move.
 *
 * @param from_volt Current voltage.
 * @param to_volt   Target voltage.
 * @return Timeout in milliseconds.
 */
static uint64_t ControlTilt_ComputeMoveTimeoutMs(float from_volt, float to_volt)
{
    float from_deg = ControlTilt_VoltToTilt(from_volt);
    float to_deg   = ControlTilt_VoltToTilt(to_volt);
    float delta    = fabsf(to_deg - from_deg);
    float sec_per_degree = (g_cal.sec_per_degree > 0.0f)
                           ? g_cal.sec_per_degree
                           : 0.5f;

    uint64_t ms = (uint64_t)(delta * sec_per_degree * 1000.0f);
    if (ms < 1000U) ms = 1000U;
    return ms + k_timeout_margin_ms;
}

/**
 * @brief Compute a conservative timeout for homing.
 *
 * @return Timeout in milliseconds.
 */
static uint64_t ControlTilt_ComputeHomeTimeoutMs(void)
{
    float span_deg = g_cal.max_angle - g_cal.min_angle;
    float sec_per_degree = (g_cal.sec_per_degree > 0.0f)
                           ? g_cal.sec_per_degree
                           : 0.5f;

    uint64_t ms = (uint64_t)(span_deg * sec_per_degree * 1000.0f);
    if (ms < 2000U) ms = 2000U;
    return ms + k_timeout_margin_ms;
}

/* -------------------------------------------------------------------------
 * Read helpers
 * ------------------------------------------------------------------------- */

/**
 * @brief Read raw tilt voltage from ADC.
 *
 * @return Voltage value, or negative on error.
 */
float ControlTilt_ReadVolt(void)
{
    int adc = ReadTiltPosition();
    if (adc < 0) return -1.0f;
    return adc / 1000.0f;
}

/**
 * @brief Read tilt position in degrees.
 *
 * @return Tilt angle in degrees.
 */
float ControlTilt_ReadDegree(void)
{
    float volts = ControlTilt_ReadVolt();
    if (volts < 0.0f) return g_last_degree;

    float deg = ControlTilt_VoltToTilt(volts);
    g_last_degree = deg;
    return deg;
}

/**
 * @brief Check whether the tilt axis is at the HOME position.
 *
 * @return 1 if homed, 0 otherwise.
 */
int ControlTilt_CheckHome(void)
{
    return ReadHomeTilt();
}

/* -------------------------------------------------------------------------
 * Homing (non-blocking)
 * ------------------------------------------------------------------------- */

/**
 * @brief Begin a non-blocking homing sequence.
 *
 * @return TILT_OK if already homed,
 *         TILT_RUNNING if homing started,
 *         TILT_ERROR on invalid state.
 */
TiltResult_t ControlTilt_BeginHome(void)
{
    /* Already homed? */
    if (ReadHomeTilt()) {
        g_tilt_is_homed = 1;
        g_home.active   = 0;
        g_machine.tilt_state = AXIS_IDLE;
        return TILT_OK;
    }

    /* Begin pulling IN */
    g_home.active = 1;
    g_machine.tilt_state = AXIS_RUNNING_TILT;

    uint64_t now = ControlTilt_NowMs();
    g_home.start_ms         = now;
    g_home.last_progress_ms = now;
    g_home.last_tick_ms     = 0;
    g_home.timeout_ms       = ControlTilt_ComputeHomeTimeoutMs();

    g_home.last_adc = ReadTiltPosition();
    if (g_home.last_adc < 0) g_home.last_adc = 0;

    RelayTilt(0, 1);  /* 0 = IN direction */

    return TILT_RUNNING;
}

/**
 * @brief Service the non-blocking homing sequence.
 *
 * @return TILT_RUNNING while moving,
 *         TILT_OK when homed,
 *         TILT_PAUSED or TILT_STOPPED on request,
 *         TILT_ERROR on fault.
 */
TiltResult_t ControlTilt_ServiceHome(void)
{
    if (!g_home.active) {
        return TILT_OK;
    }

    /* Pause request */
    if (g_machine.pause_requested) {
        RelayTilt(0, 0);
        g_home.active = 0;
        g_machine.tilt_state = AXIS_IDLE;
        return TILT_PAUSED;
    }

    /* Stop request */
    if (g_machine.stop_requested) {
        RelayTilt(0, 0);
        g_home.active = 0;
        g_machine.tilt_state = AXIS_IDLE;
        return TILT_STOPPED;
    }

    if (!ControlTilt_ShouldTick(&g_home.last_tick_ms,
                                g_cal.control_time_ms)) {
        return TILT_RUNNING;
    }

    /* Check HOME sensor */
    if (ReadHomeTilt()) {
        RelayTilt(0, 0);
        g_home.active = 0;
        g_tilt_is_homed = 1;
        g_machine.tilt_state = AXIS_IDLE;

        return TILT_OK;
    }

    int current_adc = ReadTiltPosition();
    if (current_adc < 0) {
        RelayTilt(0, 0);
        g_home.active = 0;
        g_machine.tilt_state = AXIS_IDLE;
        return TILT_ERROR;
    }

    if (abs(current_adc - g_home.last_adc) >= k_progress_adc_threshold) {
        g_home.last_adc = current_adc;
        g_home.last_progress_ms = ControlTilt_NowMs();
    }

    uint64_t now = ControlTilt_NowMs();
    if (now - g_home.last_progress_ms > k_stall_timeout_ms) {
        RelayTilt(0, 0);
        g_home.active = 0;
        g_machine.tilt_state = AXIS_IDLE;
        return TILT_ERROR;
    }

    if (now - g_home.start_ms > g_home.timeout_ms) {
        RelayTilt(0, 0);
        g_home.active = 0;
        g_machine.tilt_state = AXIS_IDLE;
        return TILT_ERROR;
    }

    return TILT_RUNNING;
}

/**
 * @brief Blocking homing routine.
 *
 * @return 0 on success, non-zero on failure.
 */
int ControlTilt_Home(void)
{
    TiltResult_t r = ControlTilt_BeginHome();
    if (r == TILT_OK) return 0;
    if (r == TILT_ERROR) return -1;

    while (1) {
        r = ControlTilt_ServiceHome();
        if (r == TILT_RUNNING) continue;
        if (r == TILT_OK)      return 0;
        return -1; /* paused or stopped or error */
    }
}

/* -------------------------------------------------------------------------
 * Non-blocking motion engine
 * ------------------------------------------------------------------------- */

/**
 * @brief Begin a non-blocking move to a target voltage.
 *
 * @param target_volt Target voltage.
 * @return TILT_OK if already at target,
 *         TILT_RUNNING if motion started,
 *         TILT_ERROR on invalid state.
 */
TiltResult_t ControlTilt_BeginMoveToVolt(float target_volt)
{
    if (!g_tilt_is_homed) {
        return TILT_ERROR;
    }

    if (target_volt < g_cal.min_volts ||
        target_volt > g_cal.max_volts)
    {
        return TILT_ERROR;
    }

    int current_adc = ReadTiltPosition();
    if (current_adc < 0) {
        return TILT_ERROR;
    }

    float current_volt = current_adc / 1000.0f;

    int up = (current_volt < target_volt) ? 1 : 0;

    float compensated_volt =
        up ? (target_volt - g_cal.stop_band_out)
           : (target_volt + g_cal.stop_band_in);

    if (compensated_volt < g_cal.min_volts)
        compensated_volt = g_cal.min_volts;

    if (compensated_volt > g_cal.max_volts)
        compensated_volt = g_cal.max_volts;

    int compensated_adc = (int)(compensated_volt * 1000.0f);

    if ((up  && current_adc >= compensated_adc) ||
        (!up && current_adc <= compensated_adc))
    {
        g_motion.active      = 0;
        g_machine.tilt_state = AXIS_IDLE;
        return TILT_OK;
    }

    g_motion.active       = 1;
    g_motion.target_adc   = compensated_adc;
    g_motion.direction_up = up;
    g_motion.last_adc     = current_adc;

    uint64_t now = ControlTilt_NowMs();
    g_motion.start_ms         = now;
    g_motion.last_progress_ms = now;
    g_motion.last_tick_ms     = 0;
    g_motion.timeout_ms       = ControlTilt_ComputeMoveTimeoutMs(current_volt,
                                                                  compensated_volt);

    g_machine.tilt_state = AXIS_RUNNING_TILT;
    g_machine.resume_requested = 0;

    RelayTilt(up, 1);

    return TILT_RUNNING;
}

/**
 * @brief Begin a non-blocking move to a target degree.
 *
 * @param target_degree Target angle in degrees.
 * @return TILT_OK if already at target,
 *         TILT_RUNNING if motion started,
 *         TILT_ERROR on invalid state.
 */
TiltResult_t ControlTilt_BeginMoveToDegree(float target_degree)
{
    if (target_degree < g_cal.min_angle ||
        target_degree > g_cal.max_angle)
    {
        return TILT_ERROR;
    }

    float target_volt = ControlTilt_TiltToVolt(target_degree);
    return ControlTilt_BeginMoveToVolt(target_volt);
}

/**
 * @brief Service the non-blocking tilt motion.
 *
 * @param actual_volt_out Optional output for current voltage.
 * @return TILT_RUNNING while moving,
 *         TILT_OK when target reached,
 *         TILT_PAUSED or TILT_STOPPED on request,
 *         TILT_ERROR on fault.
 */
TiltResult_t ControlTilt_Service(float *actual_volt_out)
{
    if (!g_motion.active) {
        if (actual_volt_out) {
            *actual_volt_out = ControlTilt_ReadVolt();
        }
        return TILT_OK;
    }

    if (g_machine.stop_requested) {
        RelayTilt(0, 0);
        g_motion.active      = 0;
        g_machine.tilt_state = AXIS_IDLE;
        if (actual_volt_out) *actual_volt_out = ControlTilt_ReadVolt();
        return TILT_STOPPED;
    }

    if (g_machine.pause_requested) {
        RelayTilt(0, 0);
        g_motion.active      = 0;
        g_machine.tilt_state = AXIS_IDLE;
        if (actual_volt_out) *actual_volt_out = ControlTilt_ReadVolt();
        return TILT_PAUSED;
    }

    if (!ControlTilt_ShouldTick(&g_motion.last_tick_ms,
                                g_cal.control_time_ms)) {
        if (actual_volt_out) *actual_volt_out = ControlTilt_ReadVolt();
        return TILT_RUNNING;
    }

    int current_adc = ReadTiltPosition();
    if (current_adc < 0) {
        RelayTilt(0, 0);
        g_motion.active      = 0;
        g_machine.tilt_state = AXIS_IDLE;
        return TILT_ERROR;
    }

    if (abs(current_adc - g_motion.last_adc) >= k_progress_adc_threshold) {
        g_motion.last_adc = current_adc;
        g_motion.last_progress_ms = ControlTilt_NowMs();
    }

    uint64_t now = ControlTilt_NowMs();
    if (now - g_motion.last_progress_ms > k_stall_timeout_ms) {
        RelayTilt(0, 0);
        g_motion.active      = 0;
        g_machine.tilt_state = AXIS_IDLE;
        return TILT_ERROR;
    }

    if (now - g_motion.start_ms > g_motion.timeout_ms) {
        RelayTilt(0, 0);
        g_motion.active      = 0;
        g_machine.tilt_state = AXIS_IDLE;
        return TILT_ERROR;
    }

    if (g_motion.direction_up && current_adc >= g_motion.target_adc) {
        RelayTilt(0, 0);
        g_motion.active      = 0;
        g_machine.tilt_state = AXIS_IDLE;
        if (actual_volt_out) *actual_volt_out = ControlTilt_ReadVolt();
        return TILT_OK;
    }

    if (!g_motion.direction_up && current_adc <= g_motion.target_adc) {
        RelayTilt(0, 0);
        g_motion.active      = 0;
        g_machine.tilt_state = AXIS_IDLE;
        if (actual_volt_out) *actual_volt_out = ControlTilt_ReadVolt();
        return TILT_OK;
    }

    if (actual_volt_out) *actual_volt_out = current_adc / 1000.0f;
    return TILT_RUNNING;
}

/* -------------------------------------------------------------------------
 * Blocking wrappers
 * ------------------------------------------------------------------------- */

/**
 * @brief Blocking move to a target voltage.
 *
 * @param target_volt Target voltage.
 * @param actual_volt_out Optional output for final voltage.
 * @return TILT_OK on success or error code on failure.
 */
TiltResult_t ControlTilt_MoveToVolt(float target_volt,
                                    float *actual_volt_out)
{
    TiltResult_t r = ControlTilt_BeginMoveToVolt(target_volt);
    if (r == TILT_ERROR || r == TILT_OK) {
        if (actual_volt_out) *actual_volt_out = ControlTilt_ReadVolt();
        return r;
    }

    while (1) {
        r = ControlTilt_Service(actual_volt_out);
        if (r != TILT_RUNNING) {
            return r;
        }
    }
}

/**
 * @brief Blocking move to a target degree.
 *
 * @param target_degree Target angle in degrees.
 * @param actual_degree_out Optional output for final angle.
 * @return TILT_OK on success or error code on failure.
 */
TiltResult_t ControlTilt_MoveToDegree(float target_degree,
                                      float *actual_degree_out)
{
    float actual_volt = 0.0f;
    TiltResult_t r = ControlTilt_MoveToVolt(
        ControlTilt_TiltToVolt(target_degree), &actual_volt);

    if (actual_degree_out)
        *actual_degree_out = ControlTilt_VoltToTilt(actual_volt);

    return r;
}

/* -------------------------------------------------------------------------
 * Misc
 * ------------------------------------------------------------------------- */

/**
 * @brief Read the current tilt position into an output pointer.
 *
 * @param degree_out Output pointer for tilt angle in degrees.
 * @return 0 on success, -1 on invalid pointer.
 */
int ControlTilt_ReadPosition(float *degree_out)
{
    if (!degree_out) return -1;
    *degree_out = ControlTilt_ReadDegree();
    return 0;
}

/**
 * @brief Pause tilt movement immediately.
 *
 * @return 0 on success.
 */
int ControlTilt_Pause(void)
{
    RelayTilt(0, 0);
    g_motion.active      = 0;
    g_home.active        = 0;
    g_machine.tilt_state = AXIS_IDLE;
    printf("[Tilt] Paused at %.2f deg\n", g_last_degree);
    return 0;
}

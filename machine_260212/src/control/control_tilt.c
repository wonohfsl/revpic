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

#include <stdio.h>
#include <unistd.h>
#include "control_tilt.h"
#include "motion.h"
#include "machine_state.h"

/* TODO (future enhancements):
 *
 * HOMING:
 *   - Apply home-offset compensation (move additional delta after prox triggers)
 *   - Enforce minimum-voltage safety (prevent over‑travel past mechanical stop)
 *
 * HOMING + MOVETO:
 *   - Add timeout protection (abort if motion exceeds allowed duration)
 *   - Add no‑movement detection (detect jammed actuator or failed relay)
 *
 * SYSTEM:
 *   - Optional heartbeat / watchdog integration
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
    int   active;
    int   target_adc;
    int   direction_up;
} g_motion = { 0, 0, 0 };

/**
 * @brief Internal homing state.
 */
static struct
{
    int active;     /**< 1 if homing is in progress */
} g_home = { 0 };

/* -------------------------------------------------------------------------
 * Calibration
 * ------------------------------------------------------------------------- */

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

float ControlTilt_TiltToVolt(float degree)
{
    float span_deg  = g_cal.max_angle - g_cal.min_angle;
    float span_volt = g_cal.max_volts - g_cal.min_volts;

    if (span_deg <= 0.0f) return g_cal.min_volts;

    return g_cal.min_volts +
           (degree - g_cal.min_angle) * (span_volt / span_deg);
}

float ControlTilt_VoltToTilt(float volts)
{
    float span_deg  = g_cal.max_angle - g_cal.min_angle;
    float span_volt = g_cal.max_volts - g_cal.min_volts;

    if (span_volt <= 0.0f) return g_cal.min_angle;

    return g_cal.min_angle +
           (volts - g_cal.min_volts) * (span_deg / span_volt);
}

/* -------------------------------------------------------------------------
 * Read helpers
 * ------------------------------------------------------------------------- */

float ControlTilt_ReadVolt(void)
{
    int adc = ReadTiltPosition();
    if (adc < 0) return -1.0f;
    return adc / 1000.0f;
}

float ControlTilt_ReadDegree(void)
{
    float volts = ControlTilt_ReadVolt();
    if (volts < 0.0f) return g_last_degree;

    float deg = ControlTilt_VoltToTilt(volts);
    g_last_degree = deg;
    return deg;
}

int ControlTilt_CheckHome(void)
{
    return ReadHomeTilt();
}

/* -------------------------------------------------------------------------
 * Homing (non-blocking)
 * ------------------------------------------------------------------------- */

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

    RelayTilt(0, 1);  /* 0 = IN direction */

    return TILT_RUNNING;
}

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

    /* Sleep one control interval */
    usleep(g_cal.control_time_ms * 1000);

    /* Check HOME sensor */
    if (ReadHomeTilt()) {
        RelayTilt(0, 0);
        g_home.active = 0;
        g_tilt_is_homed = 1;
        g_machine.tilt_state = AXIS_IDLE;

        /* TODO (future):
         * - Move extra delta (home offset compensation)
         * - Ensure not below min voltage (safety)
         * - Timeout protection
         * - No-movement detection
         */

        return TILT_OK;
    }

    return TILT_RUNNING;
}

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

// TEMP
// void ControlTilt_ForceHomedForTest(void)
// {
//     g_tilt_is_homed = 1;
// }

/* -------------------------------------------------------------------------
 * Non-blocking motion engine (unchanged)
 * ------------------------------------------------------------------------- */

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

    g_machine.tilt_state = AXIS_RUNNING_TILT;
    g_machine.resume_requested = 0;

    RelayTilt(up, 1);

    return TILT_RUNNING;
}

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

    usleep(g_cal.control_time_ms * 1000);

    int current_adc = ReadTiltPosition();
    if (current_adc < 0) {
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
 * Blocking wrappers (unchanged)
 * ------------------------------------------------------------------------- */

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

int ControlTilt_ReadPosition(float *degree_out)
{
    if (!degree_out) return -1;
    *degree_out = ControlTilt_ReadDegree();
    return 0;
}

int ControlTilt_Pause(void)
{
    RelayTilt(0, 0);
    g_motion.active      = 0;
    g_home.active        = 0;
    g_machine.tilt_state = AXIS_IDLE;
    printf("[Tilt] Paused at %.2f deg\n", g_last_degree);
    return 0;
}

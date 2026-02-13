/**
 * @file control_tilt.h
 * @brief High-level control API for the tilt (T-axis) actuator.
 *
 * Supports:
 *   - Homing
 *   - Movement by voltage and degree
 *   - Non-blocking motion engine (Begin/Service)
 *   - Pause/stop-aware motion
 *   - Stop-band compensation (in/out, in volts)
 */

/* TODO:
 * - Add homing timeout and jam detection configuration.
 * - Expose calibration persistence helpers.
 */

#ifndef CONTROL_TILT_H
#define CONTROL_TILT_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Calibration structure for the tilt axis.
 *
 * stop_band_in:
 *      Compensation (volts) when pulling IN.
 *
 * stop_band_out:
 *      Compensation (volts) when pulling OUT.
 */
typedef struct
{
    int   seat_time_ms;        /**< Seat time (ms), reserved for future use */
    float minimum_volts;       /**< Minimum valid tilt sensor voltage */
    float maximum_volts;       /**< Maximum valid tilt sensor voltage */
    float deadband;            /**< Unused placeholder */
    float stop_band_in;        /**< Stop-band compensation when pulling IN (V) */
    float stop_band_out;       /**< Stop-band compensation when pulling OUT (V) */
    float sec_per_degree;      /**< Unused placeholder (speed) */
    float max_angle;           /**< Maximum allowed tilt angle (degree) */
    float min_angle;           /**< Minimum allowed tilt angle (degree) */
    int   control_time_ms;     /**< Motion loop sampling time (ms) */
} TiltCalibration_t;

/**
 * @brief Result codes for tilt motion commands.
 */
typedef enum
{
    TILT_OK = 0,     /**< Motion completed successfully */
    TILT_RUNNING,    /**< Motion is in progress (non-blocking API) */
    TILT_PAUSED,     /**< Motion paused due to pause request */
    TILT_STOPPED,    /**< Motion stopped due to stop request */
    TILT_ERROR       /**< Motion failed due to error or invalid input */
} TiltResult_t;

/**
 * @brief Apply calibration values to the tilt controller.
 *
 * @param cfg Pointer to calibration structure (must not be NULL).
 */
void ControlTilt_ApplyCalibration(const TiltCalibration_t *cfg);

/**
 * @brief Convert tilt degrees to sensor voltage.
 *
 * @param degree Tilt angle in degrees.
 * @return Voltage corresponding to the given angle.
 */
float ControlTilt_TiltToVolt(float degree);

/**
 * @brief Convert sensor voltage to tilt degrees.
 *
 * @param volts Sensor voltage.
 * @return Tilt angle in degrees.
 */
float ControlTilt_VoltToTilt(float volts);

/**
 * @brief Read raw tilt voltage from ADC.
 *
 * @return Voltage value, or negative on error.
 */
float ControlTilt_ReadVolt(void);

/**
 * @brief Read tilt position in degrees.
 *
 * @return Tilt angle in degrees.
 */
float ControlTilt_ReadDegree(void);

/**
 * @brief Check whether the tilt axis is at the HOME position.
 *
 * Uses ReadHomeTilt() from motion.c.
 *
 * @return 1 if homed, 0 otherwise.
 */
int ControlTilt_CheckHome(void);

/**
 * @brief Begin a non-blocking homing sequence.
 *
 * Behavior:
 *   - If HOME sensor is already active -> homed immediately.
 *   - Otherwise, pulls IN until HOME sensor becomes active.
 *   - Pause/stop requests are honored.
 *
 * NOTE: This is a minimal homing routine. Future enhancements:
 *   - Home offset compensation
 *   - Minimum-voltage safety check
 *   - Timeout protection
 *   - No-movement detection
 *
 * @return TILT_OK if already homed,
 *         TILT_RUNNING if motion started,
 *         TILT_ERROR on invalid state.
 */
TiltResult_t ControlTilt_BeginHome(void);

/**
 * @brief Service the non-blocking homing sequence.
 *
 * @return TILT_RUNNING while moving,
 *         TILT_OK when homed,
 *         TILT_PAUSED or TILT_STOPPED on request,
 *         TILT_ERROR on fault.
 */
TiltResult_t ControlTilt_ServiceHome(void);

/**
 * @brief Blocking homing routine.
 *
 * Wraps BeginHome() + ServiceHome() until completion.
 *
 * @return 0 on success, non-zero on failure.
 */
int ControlTilt_Home(void);

// TEMP
//void ControlTilt_ForceHomedForTest(void);

/**
 * @brief Begin a non-blocking move to a target voltage.
 */
TiltResult_t ControlTilt_BeginMoveToVolt(float target_volt);

/**
 * @brief Begin a non-blocking move to a target degree.
 */
TiltResult_t ControlTilt_BeginMoveToDegree(float target_degree);

/**
 * @brief Service the non-blocking tilt motion.
 */
TiltResult_t ControlTilt_Service(float *actual_volt_out);

/**
 * @brief Blocking move to a target voltage.
 */
TiltResult_t ControlTilt_MoveToVolt(float target_volt,
                                    float *actual_volt_out);

/**
 * @brief Blocking move to a target degree.
 */
TiltResult_t ControlTilt_MoveToDegree(float target_degree,
                                      float *actual_degree_out);

/**
 * @brief Read the current tilt position into an output pointer.
 */
int ControlTilt_ReadPosition(float *degree_out);

/**
 * @brief Pause tilt movement immediately.
 */
int ControlTilt_Pause(void);

#ifdef __cplusplus
}
#endif

#endif /* CONTROL_TILT_H */

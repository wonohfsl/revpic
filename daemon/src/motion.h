/**
 * @file motion.h
 * @brief Mid-level motion control abstraction for RevPi-based machine I/O.
 *
 * This module provides semantic, machine-specific access to:
 *  - E‑STOP input
 *  - Home sensors (rotate, tilt)
 *  - Tilt position analog input
 *  - Relay outputs for tilt and rotate actuators
 *
 * It sits above the low-level HAL (mio.c, ro.c) and below high-level
 * application logic (state machines, IPC-driven control loops).
 */

#ifndef MOTION_H
#define MOTION_H

#include <stdint.h>

/* -------------------------------------------------------------------------
 * Channel Mapping (Machine-Specific)
 * -------------------------------------------------------------------------
 *
 * DIGITAL INPUTS (mio_get_di)
 *   DI_PROXI_ROTATE  → DigitalInput_1  (Home sensor for rotate axis)
 *   DI_PROXI_TILT    → DigitalInput_2  (Home sensor for tilt axis)
 *   DI_ESTOP         → DigitalInput_4  (Emergency stop button)
 *
 * ANALOG INPUTS (mio_get_ai)
 *   AI_TILT_POS      → AnalogInput_1   (Tilt position feedback)
 *
 * RELAY OUTPUTS (ro_set_ro)
 *   RO_TILT_DIR      → RelayOutput_4   (Tilt direction: up=1, down=0)
 *   RO_TILT_EN       → RelayOutput_3   (Tilt enable)
 *   RO_ROTATE_DIR    → RelayOutput_2   (Rotate direction: cw=1, ccw=0)
 *   RO_ROTATE_EN     → RelayOutput_1   (Rotate enable)
 */

/* Digital Inputs */
#define DI_PROXI_ROTATE   1
#define DI_PROXI_TILT     2
#define DI_ESTOP          4

/* Analog Inputs */
#define AI_TILT_POS       1

/* Relay Outputs */
#define RO_TILT_DIR       4
#define RO_TILT_EN        3
#define RO_ROTATE_DIR     2
#define RO_ROTATE_EN      1

/* -------------------------------------------------------------------------
 * Mid-Level Read Functions
 * ------------------------------------------------------------------------- */

/**
 * @brief Read the emergency stop button state.
 *
 * @return 0 = not pressed, 1 = pressed, -1 = read error.
 */
int ReadEStopButton(void);

/**
 * @brief Read the rotate-axis home sensor.
 *
 * @return 0 = not at home, 1 = at home, -1 = read error.
 */
int ReadHomeRotate(void);

/**
 * @brief Read the tilt-axis home sensor.
 *
 * @return 0 = not at home, 1 = at home, -1 = read error.
 */
int ReadHomeTilt(void);

/**
 * @brief Read the tilt position analog input (single raw ADC read).
 *
 * @return Raw ADC value (0–10000) or -1 on error.
 */
int ReadTiltADC(void);

/**
 * @brief Read the tilt position with filtering.
 *
 * This function reads the ADC three times with 1 ms spacing and returns
 * the integer average. The ADC range is 0–10000 corresponding to 0–10 V.
 *
 * NOTE:
 *   - The ADC cannot reliably measure below ~25 counts (~0.025 V).
 *   - The linear actuator's valid signal range is 0.95–9.23 V
 *     (≈950–9230 counts). Values below ~950 counts are outside the
 *     actuator's meaningful range and should be treated as "0°" or invalid.
 *
 * @return Filtered ADC value (0–10000) or -1 on error.
 */
int ReadTiltPosition(void);

/* -------------------------------------------------------------------------
 * Mid-Level Relay Control
 * ------------------------------------------------------------------------- */

void RelayRotate(int cw, int on);
void RelayTilt(int up, int on);

#endif /* MOTION_H */

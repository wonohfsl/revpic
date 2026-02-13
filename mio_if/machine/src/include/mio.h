/**
 * @file mio.h
 * @brief Hardware Abstraction Layer (HAL) for RevPi MIO module.
 *
 * This module provides high-level, channel-based access to:
 * - Digital Inputs (DI1–DI4)
 * - Digital Outputs (DO1–DO4)
 * - Analog Inputs (AI1–AI8)
 * - Analog Outputs (AO1–AO8)
 *
 * All functions internally use piControl ioctl access (no mmap).
 * Offsets and bit positions are defined in @ref mio_addr.h.
 */

#ifndef MIO_H
#define MIO_H

#include <stdint.h>

/**
 * @brief Initialize the MIO HAL and open piControl device.
 *
 * This must be called once before using any other MIO functions.
 *
 * @return 0 on success, -1 on failure.
 */
int mio_init(void);

/**
 * @brief Read a digital input channel (DI1–DI4).
 *
 * @param ch Channel number (1–4).
 * @return 0 or 1 on success, -1 on invalid channel or read error.
 */
int mio_get_di(int ch);

/**
 * @brief Set a digital output channel (DO1–DO4).
 *
 * @param ch Channel number (1–4).
 * @param value 0 = OFF, 1 = ON.
 * @return 0 on success, -1 on error.
 */
int mio_set_do(int ch, int value);

/**
 * @brief Read a digital output channel (DO1–DO4).
 *
 * @param ch Channel number (1–4).
 * @return 0 or 1 on success, -1 on invalid channel or read error.
 */
int mio_get_do(int ch);

/**
 * @brief Read an analog input channel (AI1–AI8).
 *
 * @param ch Channel number (1–8).
 * @return Raw 16-bit value (0–10000) on success, -1 on error.
 */
int mio_get_ai(int ch);

/**
 * @brief Read an analog output channel (AO1–AO8).
 *
 * @param ch Channel number (1–8).
 * @return Raw 16-bit value (0–10000) on success, -1 on error.
 */
int mio_get_ao(int ch);

/**
 * @brief Set an analog output channel (AO1–AO8).
 *
 * @param ch Channel number (1–8).
 * @param value Raw 16-bit output value (0–10000).
 * @return 0 on success, -1 on error.
 */
int mio_set_ao(int ch, uint16_t value);

#endif /* MIO_H */

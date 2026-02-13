/**
 * @file ro.h
 * @brief Hardware Abstraction Layer (HAL) for RevPi RO module.
 */

#ifndef RO_H
#define RO_H

#include <stdint.h>

/**
 * @brief Initialize RO HAL (opens piControl).
 */
int ro_init(void);

/**
 * @brief Read relay output channel (RO1–RO4).
 *
 * @param ch Channel number (1–4)
 * @return 0 or 1 on success, -1 on error
 */
int ro_get_ro(int ch);

/**
 * @brief Set relay output channel (RO1–RO4).
 *
 * @param ch Channel number (1–4)
 * @param value 0 = OFF, 1 = ON
 * @return 0 on success, -1 on error
 */
int ro_set_ro(int ch, int value);

/**
 * @brief Retrieve offset/bit/len for a relay channel.
 *
 * @param ch Channel number (1–4)
 * @param offset Output: byte offset
 * @param bit Output: bit position
 * @param len Output: bit length (always 1)
 * @return 0 on success, -1 on error
 */
int ro_get_addr(int ch, int *offset, int *bit, int *len);

#endif

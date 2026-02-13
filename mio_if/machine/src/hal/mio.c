/**
 * @file mio.c
 * @brief Implementation of the RevPi MIO Hardware Abstraction Layer (HAL).
 *
 * This module wraps piControl read/write operations into channel-based
 * functions for digital and analog I/O. All offsets and bit positions
 * are defined in @ref mio_addr.h.
 */

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

#include "piControl.h"
#include "piControlIf.h"
#include "mio_addr.h"
#include "mio.h"

/* -------------------------------------------------------------------------
 * Initialization
 * ------------------------------------------------------------------------- */

/**
 * @brief Initialize the MIO HAL by opening the piControl device.
 */
int mio_init(void)
{
    return piControlOpen();
}

/* -------------------------------------------------------------------------
 * Digital Inputs
 * ------------------------------------------------------------------------- */

/**
 * @brief Read a digital input channel (DI1–DI4).
 */
int mio_get_di(int ch)
{
    uint8_t value = 0;

    switch (ch) {
        case 1: return piControlRead(DI1_OFFSET, 1, &value) < 0 ? -1 : (value >> DI1_BIT) & 1;
        case 2: return piControlRead(DI2_OFFSET, 1, &value) < 0 ? -1 : (value >> DI2_BIT) & 1;
        case 3: return piControlRead(DI3_OFFSET, 1, &value) < 0 ? -1 : (value >> DI3_BIT) & 1;
        case 4: return piControlRead(DI4_OFFSET, 1, &value) < 0 ? -1 : (value >> DI4_BIT) & 1;
    }
    return -1;
}

/* -------------------------------------------------------------------------
 * Digital Outputs
 * ------------------------------------------------------------------------- */

/**
 * @brief Read a digital output channel (DO1–DO4).
 */
int mio_get_do(int ch)
{
    uint8_t value = 0;

    switch (ch) {
        case 1: return piControlRead(DO1_OFFSET, 1, &value) < 0 ? -1 : (value >> DO1_BIT) & 1;
        case 2: return piControlRead(DO2_OFFSET, 1, &value) < 0 ? -1 : (value >> DO2_BIT) & 1;
        case 3: return piControlRead(DO3_OFFSET, 1, &value) < 0 ? -1 : (value >> DO3_BIT) & 1;
        case 4: return piControlRead(DO4_OFFSET, 1, &value) < 0 ? -1 : (value >> DO4_BIT) & 1;
    }
    return -1;
}

/**
 * @brief Set a digital output channel (DO1–DO4) without overwriting other DO bits.
 *
 * This function originally overwrote the entire output byte at DOx_OFFSET
 * using a freshly constructed value (0 or 1<<bit). Because DO1–DO4 share
 * the same byte in the process image, writing a new byte cleared all other
 * DO bits. For example, setting DO1 would unintentionally clear DO2–DO4.
 *
 * FIX:
 *   - Read the existing byte from DOx_OFFSET.
 *   - Modify only the target bit using bitmask operations.
 *   - Write the updated byte back.
 *
 * This preserves the state of all other DO channels while updating only
 * the requested one.
 *
 * @param ch Channel number (1–4)
 * @param value 0 = OFF, 1 = ON
 * @return 0 on success, -1 on error
 */
int mio_set_do(int ch, int value)
{
    uint8_t byte = 0;

    switch (ch) {
        case 1:
            piControlRead(DO1_OFFSET, 1, &byte);          // read existing byte
            if (value) byte |=  (1 << DO1_BIT);           // set bit
            else       byte &= ~(1 << DO1_BIT);           // clear bit
            return piControlWrite(DO1_OFFSET, 1, &byte);

        case 2:
            piControlRead(DO2_OFFSET, 1, &byte);
            if (value) byte |=  (1 << DO2_BIT);
            else       byte &= ~(1 << DO2_BIT);
            return piControlWrite(DO2_OFFSET, 1, &byte);

        case 3:
            piControlRead(DO3_OFFSET, 1, &byte);
            if (value) byte |=  (1 << DO3_BIT);
            else       byte &= ~(1 << DO3_BIT);
            return piControlWrite(DO3_OFFSET, 1, &byte);

        case 4:
            piControlRead(DO4_OFFSET, 1, &byte);
            if (value) byte |=  (1 << DO4_BIT);
            else       byte &= ~(1 << DO4_BIT);
            return piControlWrite(DO4_OFFSET, 1, &byte);
    }
    return -1;
}

/* -------------------------------------------------------------------------
 * Analog Inputs
 * ------------------------------------------------------------------------- */

/**
 * @brief Read an analog input channel (AI1–AI8).
 */
int mio_get_ai(int ch)
{
    uint16_t value = 0;

    switch (ch) {
        case 1: return piControlRead(AI1_OFFSET, 2, (uint8_t *)&value) < 0 ? -1 : value;
        case 2: return piControlRead(AI2_OFFSET, 2, (uint8_t *)&value) < 0 ? -1 : value;
        case 3: return piControlRead(AI3_OFFSET, 2, (uint8_t *)&value) < 0 ? -1 : value;
        case 4: return piControlRead(AI4_OFFSET, 2, (uint8_t *)&value) < 0 ? -1 : value;
        case 5: return piControlRead(AI5_OFFSET, 2, (uint8_t *)&value) < 0 ? -1 : value;
        case 6: return piControlRead(AI6_OFFSET, 2, (uint8_t *)&value) < 0 ? -1 : value;
        case 7: return piControlRead(AI7_OFFSET, 2, (uint8_t *)&value) < 0 ? -1 : value;
        case 8: return piControlRead(AI8_OFFSET, 2, (uint8_t *)&value) < 0 ? -1 : value;
    }
    return -1;
}

/* -------------------------------------------------------------------------
 * Analog Outputs
 * ------------------------------------------------------------------------- */

/**
 * @brief Read an analog output channel (AO1–AO8).
 */
int mio_get_ao(int ch)
{
    uint16_t value = 0;

    switch (ch) {
        case 1: return piControlRead(AO1_OFFSET, 2, (uint8_t *)&value) < 0 ? -1 : value;
        case 2: return piControlRead(AO2_OFFSET, 2, (uint8_t *)&value) < 0 ? -1 : value;
        case 3: return piControlRead(AO3_OFFSET, 2, (uint8_t *)&value) < 0 ? -1 : value;
        case 4: return piControlRead(AO4_OFFSET, 2, (uint8_t *)&value) < 0 ? -1 : value;
        case 5: return piControlRead(AO5_OFFSET, 2, (uint8_t *)&value) < 0 ? -1 : value;
        case 6: return piControlRead(AO6_OFFSET, 2, (uint8_t *)&value) < 0 ? -1 : value;
        case 7: return piControlRead(AO7_OFFSET, 2, (uint8_t *)&value) < 0 ? -1 : value;
        case 8: return piControlRead(AO8_OFFSET, 2, (uint8_t *)&value) < 0 ? -1 : value;
    }
    return -1;
}

/**
 * @brief Set an analog output channel (AO1–AO8).
 */
int mio_set_ao(int ch, uint16_t value)
{
    switch (ch) {
        case 1: return piControlWrite(AO1_OFFSET, 2, (uint8_t *)&value);
        case 2: return piControlWrite(AO2_OFFSET, 2, (uint8_t *)&value);
        case 3: return piControlWrite(AO3_OFFSET, 2, (uint8_t *)&value);
        case 4: return piControlWrite(AO4_OFFSET, 2, (uint8_t *)&value);
        case 5: return piControlWrite(AO5_OFFSET, 2, (uint8_t *)&value);
        case 6: return piControlWrite(AO6_OFFSET, 2, (uint8_t *)&value);
        case 7: return piControlWrite(AO7_OFFSET, 2, (uint8_t *)&value);
        case 8: return piControlWrite(AO8_OFFSET, 2, (uint8_t *)&value);
    }
    return -1;
}

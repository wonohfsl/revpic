/**
 * @file ro.c
 * @brief Implementation of the RevPi RO Hardware Abstraction Layer (HAL).
 */

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

#include "piControl.h"
#include "piControlIf.h"
#include "ro_addr.h"
#include "ro.h"

int ro_init(void)
{
    return piControlOpen();
}

int ro_get_addr(int ch, int *offset, int *bit, int *len)
{
    if (!offset || !bit || !len)
        return -1;

    switch (ch) {
        case 1: *offset = RO1_OFFSET; *bit = RO1_BIT; *len = 1; return 0;
        case 2: *offset = RO2_OFFSET; *bit = RO2_BIT; *len = 1; return 0;
        case 3: *offset = RO3_OFFSET; *bit = RO3_BIT; *len = 1; return 0;
        case 4: *offset = RO4_OFFSET; *bit = RO4_BIT; *len = 1; return 0;
    }
    return -1;
}

int ro_get_ro(int ch)
{
    uint8_t value = 0;

    switch (ch) {
        case 1: return piControlRead(RO1_OFFSET, 1, &value) < 0 ? -1 : (value >> RO1_BIT) & 1;
        case 2: return piControlRead(RO2_OFFSET, 1, &value) < 0 ? -1 : (value >> RO2_BIT) & 1;
        case 3: return piControlRead(RO3_OFFSET, 1, &value) < 0 ? -1 : (value >> RO3_BIT) & 1;
        case 4: return piControlRead(RO4_OFFSET, 1, &value) < 0 ? -1 : (value >> RO4_BIT) & 1;
    }
    return -1;
}

/**
 * @brief Set a relay output channel (RO1–RO4) without overwriting other RO bits.
 *
 * The original implementation constructed a new byte containing only the target
 * relay bit (1 << ROx_BIT) or 0, and wrote that byte directly to ROx_OFFSET.
 * Because all four RO channels share the same process‑image byte, this caused
 * unintended side effects: writing ROx cleared the state of the other three
 * relay bits. For example, enabling RO_TILT_EN would clear RO_TILT_DIR.
 *
 * FIX:
 *   - Read the existing byte from ROx_OFFSET.
 *   - Modify only the target bit using bitmask operations.
 *   - Write the updated byte back.
 *
 * This preserves the state of all other relay channels while updating only
 * the requested one.
 *
 * @param ch Channel number (1–4)
 * @param value 0 = OFF, 1 = ON
 * @return 0 on success, -1 on error
 */
int ro_set_ro(int ch, int value)
{
    uint8_t byte = 0;

    switch (ch) {
        case 1:
            piControlRead(RO1_OFFSET, 1, &byte);    // read existing byte
            if (value)
                byte |=  (1 << RO1_BIT);            // set bit
            else
                byte &= ~(1 << RO1_BIT);            // clear bit
            return piControlWrite(RO1_OFFSET, 1, &byte);

        case 2:
            piControlRead(RO2_OFFSET, 1, &byte);
            if (value)
                byte |=  (1 << RO2_BIT);
            else
                byte &= ~(1 << RO2_BIT);
            return piControlWrite(RO2_OFFSET, 1, &byte);

        case 3:
            piControlRead(RO3_OFFSET, 1, &byte);
            if (value)
                byte |=  (1 << RO3_BIT);
            else
                byte &= ~(1 << RO3_BIT);
            return piControlWrite(RO3_OFFSET, 1, &byte);

        case 4:
            piControlRead(RO4_OFFSET, 1, &byte);
            if (value)
                byte |=  (1 << RO4_BIT);
            else
                byte &= ~(1 << RO4_BIT);
            return piControlWrite(RO4_OFFSET, 1, &byte);
    }
    return -1;
}


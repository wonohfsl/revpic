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

int ro_set_ro(int ch, int value)
{
    uint8_t byte = 0;

    switch (ch) {
        case 1: byte = value ? (1 << RO1_BIT) : 0; return piControlWrite(RO1_OFFSET, 1, &byte);
        case 2: byte = value ? (1 << RO2_BIT) : 0; return piControlWrite(RO2_OFFSET, 1, &byte);
        case 3: byte = value ? (1 << RO3_BIT) : 0; return piControlWrite(RO3_OFFSET, 1, &byte);
        case 4: byte = value ? (1 << RO4_BIT) : 0; return piControlWrite(RO4_OFFSET, 1, &byte);
    }
    return -1;
}

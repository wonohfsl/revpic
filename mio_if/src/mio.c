#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

#include "piControl.h"
#include "piControlIf.h"
#include "mio_addr.h"
#include "mio.h"

int mio_init(void)
{
    return piControlOpen();
}

/* ============================
   DIGITAL INPUTS
   ============================ */
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

/* ============================
   DIGITAL OUTPUTS
   ============================ */
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

int mio_set_do(int ch, int value)
{
    uint8_t byte = 0;

    switch (ch) {
        case 1:
            byte = value ? (1 << DO1_BIT) : 0;
            return piControlWrite(DO1_OFFSET, 1, &byte);
        case 2:
            byte = value ? (1 << DO2_BIT) : 0;
            return piControlWrite(DO2_OFFSET, 1, &byte);
        case 3:
            byte = value ? (1 << DO3_BIT) : 0;
            return piControlWrite(DO3_OFFSET, 1, &byte);
        case 4:
            byte = value ? (1 << DO4_BIT) : 0;
            return piControlWrite(DO4_OFFSET, 1, &byte);
    }
    return -1;
}

/* ============================
   ANALOG INPUTS
   ============================ */
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

/* ============================
   ANALOG OUTPUTS
   ============================ */
   
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

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>

#include "piControl.h"
#include "piControlIf.h"
#include "mio_addr.h"
#include "revpi.h"

/* ============================
   DIO IMPLEMENTATION
   ============================ */
static int dio_get(struct revpi *rpi, int ch, int *value)
{
    uint8_t byte = 0;

    switch (ch) {
        case 1: piControlRead(DI1_OFFSET, 1, &byte); *value = (byte >> DI1_BIT) & 1; return 0;
        case 2: piControlRead(DI2_OFFSET, 1, &byte); *value = (byte >> DI2_BIT) & 1; return 0;
        case 3: piControlRead(DI3_OFFSET, 1, &byte); *value = (byte >> DI3_BIT) & 1; return 0;
        case 4: piControlRead(DI4_OFFSET, 1, &byte); *value = (byte >> DI4_BIT) & 1; return 0;
    }
    return -1;
}

static int dio_set(struct revpi *rpi, int ch, int value)
{
    uint8_t byte = 0;

    switch (ch) {
        case 1: byte = value << DO1_BIT; return piControlWrite(DO1_OFFSET, 1, &byte);
        case 2: byte = value << DO2_BIT; return piControlWrite(DO2_OFFSET, 1, &byte);
        case 3: byte = value << DO3_BIT; return piControlWrite(DO3_OFFSET, 1, &byte);
        case 4: byte = value << DO4_BIT; return piControlWrite(DO4_OFFSET, 1, &byte);
    }
    return -1;
}

/* ============================
   AI IMPLEMENTATION
   ============================ */
static int ai_read(struct revpi *rpi, int ch, int *value)
{
    uint16_t v = 0;

    switch (ch) {
        case 1: piControlRead(AI1_OFFSET, 2, (uint8_t *)&v); *value = v; return 0;
        case 2: piControlRead(AI2_OFFSET, 2, (uint8_t *)&v); *value = v; return 0;
        case 3: piControlRead(AI3_OFFSET, 2, (uint8_t *)&v); *value = v; return 0;
        case 4: piControlRead(AI4_OFFSET, 2, (uint8_t *)&v); *value = v; return 0;
        case 5: piControlRead(AI5_OFFSET, 2, (uint8_t *)&v); *value = v; return 0;
        case 6: piControlRead(AI6_OFFSET, 2, (uint8_t *)&v); *value = v; return 0;
        case 7: piControlRead(AI7_OFFSET, 2, (uint8_t *)&v); *value = v; return 0;
        case 8: piControlRead(AI8_OFFSET, 2, (uint8_t *)&v); *value = v; return 0;
    }
    return -1;
}

/* ============================
   AO IMPLEMENTATION
   ============================ */
static int ao_write(struct revpi *rpi, int ch, uint16_t value)
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

/* ============================
   CLOSE METHOD
   ============================ */
static void revpi_close(struct revpi *rpi)
{
    piControlClose();
}

/* ============================
   CONSTRUCTOR
   ============================ */
revpi_t revpi_open(const char *dev)
{
    revpi_t rpi = {0};

    if (piControlOpen() < 0) {
        printf("Failed to open piControl\n");
    }

    rpi.fd = 1;  // not used, but kept for structure completeness

    rpi.dio.get = dio_get;
    rpi.dio.set = dio_set;

    rpi.ai.read = ai_read;

    rpi.ao.write = ao_write;

    rpi.close = revpi_close;

    return rpi;
}

#ifndef REVPI_H
#define REVPI_H

#include <stdint.h>

/* Forward declaration */
struct revpi;

/* ============================
   DIO METHOD TABLE
   ============================ */
typedef struct {
    int (*get)(struct revpi *rpi, int ch, int *value);
    int (*set)(struct revpi *rpi, int ch, int value);
} revpi_dio_t;

/* ============================
   AI METHOD TABLE
   ============================ */
typedef struct {
    int (*read)(struct revpi *rpi, int ch, int *value);
} revpi_ai_t;

/* ============================
   AO METHOD TABLE
   ============================ */
typedef struct {
    int (*write)(struct revpi *rpi, int ch, uint16_t value);
} revpi_ao_t;

/* ============================
   MAIN DEVICE OBJECT
   ============================ */
typedef struct revpi {
    int fd;            // /dev/piControl0 handle

    revpi_dio_t dio;
    revpi_ai_t  ai;
    revpi_ao_t  ao;

    void (*close)(struct revpi *rpi);
} revpi_t;

/* Constructor */
revpi_t revpi_open(const char *dev);

#endif

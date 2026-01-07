#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "piControl.h"
#include "revpi_connect4_mio_map.h"

/* ------------------------------------------------------------------------- */
/* Internal file descriptor                                                   */
/* ------------------------------------------------------------------------- */

static int g_fd = -1;

/* ------------------------------------------------------------------------- */
/* Initialization / Shutdown                                                  */
/* ------------------------------------------------------------------------- */

int mio_init(void)
{
    g_fd = open("/dev/piControl0", O_RDWR);
    if (g_fd < 0) {
        perror("open /dev/piControl0");
        return -1;
    }
    return 0;
}

void mio_close(void)
{
    if (g_fd >= 0)
        close(g_fd);
    g_fd = -1;
}

/* ------------------------------------------------------------------------- */
/* Low-level helpers                                                          */
/* ------------------------------------------------------------------------- */

static int mio_get_bit(uint16_t offset, uint8_t bit)
{
    SPIValue v;
    v.i16uAddress = offset;
    v.i8uBit      = bit;

    if (ioctl(g_fd, KB_GET_VALUE, &v) < 0) {
        perror("KB_GET_VALUE");
        return -1;
    }
    return v.i8uValue;
}

static int mio_set_bit(uint16_t offset, uint8_t bit, uint8_t value)
{
    SPIValue v;
    v.i16uAddress = offset;
    v.i8uBit      = bit;
    v.i8uValue    = value ? 1 : 0;

    if (ioctl(g_fd, KB_SET_VALUE, &v) < 0) {
        perror("KB_SET_VALUE");
        return -1;
    }
    return 0;
}

static int mio_get_word(uint16_t offset)
{
    SPIValue v;
    v.i16uAddress = offset;
    v.i8uBit      = 0;   /* unused for 16-bit */

    if (ioctl(g_fd, KB_GET_VALUE, &v) < 0) {
        perror("KB_GET_VALUE");
        return -1;
    }
    return v.i16uValue;
}

static int mio_set_word(uint16_t offset, uint16_t value)
{
    SPIValue v;
    v.i16uAddress = offset;
    v.i8uBit      = 0;
    v.i16uValue   = value;

    if (ioctl(g_fd, KB_SET_VALUE, &v) < 0) {
        perror("KB_SET_VALUE");
        return -1;
    }
    return 0;
}

/* ------------------------------------------------------------------------- */
/* Public API: Digital Inputs (DI1..DI8)                                     */
/* ------------------------------------------------------------------------- */

int mio_get_di(int ch)
{
    switch (ch) {
        case 1: return mio_get_bit(MIO_IN_DI1_OFFSET, MIO_IN_DI1_BIT);
        case 2: return mio_get_bit(MIO_IN_DI2_OFFSET, MIO_IN_DI2_BIT);
        case 3: return mio_get_bit(MIO_IN_DI3_OFFSET, MIO_IN_DI3_BIT);
        case 4: return mio_get_bit(MIO_IN_DI4_OFFSET, MIO_IN_DI4_BIT);
        case 5: return mio_get_bit(MIO_IN_DI5_OFFSET, MIO_IN_DI5_BIT);
        case 6: return mio_get_bit(MIO_IN_DI6_OFFSET, MIO_IN_DI6_BIT);
        case 7: return mio_get_bit(MIO_IN_DI7_OFFSET, MIO_IN_DI7_BIT);
        case 8: return mio_get_bit(MIO_IN_DI8_OFFSET, MIO_IN_DI8_BIT);
    }
    return -1;
}

/* ------------------------------------------------------------------------- */
/* Public API: Digital Outputs (DO1..DO8)                                    */
/* ------------------------------------------------------------------------- */

int mio_set_do(int ch, int value)
{
    switch (ch) {
        case 1: return mio_set_bit(MIO_OUT_DO1_OFFSET, MIO_OUT_DO1_BIT, value);
        case 2: return mio_set_bit(MIO_OUT_DO2_OFFSET, MIO_OUT_DO2_BIT, value);
        case 3: return mio_set_bit(MIO_OUT_DO3_OFFSET, MIO_OUT_DO3_BIT, value);
        case 4: return mio_set_bit(MIO_OUT_DO4_OFFSET, MIO_OUT_DO4_BIT, value);
        case 5: return mio_set_bit(MIO_OUT_DO5_OFFSET, MIO_OUT_DO5_BIT, value);
        case 6: return mio_set_bit(MIO_OUT_DO6_OFFSET, MIO_OUT_DO6_BIT, value);
        case 7: return mio_set_bit(MIO_OUT_DO7_OFFSET, MIO_OUT_DO7_BIT, value);
        case 8: return mio_set_bit(MIO_OUT_DO8_OFFSET, MIO_OUT_DO8_BIT, value);
    }
    return -1;
}

/* ------------------------------------------------------------------------- */
/* Public API: Analog Inputs (AI1..AI8)                                      */
/* ------------------------------------------------------------------------- */

int mio_get_ai(int ch)
{
    switch (ch) {
        case 1: return mio_get_word(MIO_IN_AI1_OFFSET);
        case 2: return mio_get_word(MIO_IN_AI2_OFFSET);
        case 3: return mio_get_word(MIO_IN_AI3_OFFSET);
        case 4: return mio_get_word(MIO_IN_AI4_OFFSET);
        case 5: return mio_get_word(MIO_IN_AI5_OFFSET);
        case 6: return mio_get_word(MIO_IN_AI6_OFFSET);
        case 7: return mio_get_word(MIO_IN_AI7_OFFSET);
        case 8: return mio_get_word(MIO_IN_AI8_OFFSET);
    }
    return -1;
}

/* ------------------------------------------------------------------------- */
/* Public API: Memory registers (IO_Mode, PulseLength, etc.)                 */
/* ------------------------------------------------------------------------- */

int mio_get_mem8(uint16_t abs_offset)
{
    return mio_get_bit(abs_offset, 0); /* 8-bit read uses i8uValue */
}

int mio_set_mem8(uint16_t abs_offset, uint8_t value)
{
    return mio_set_bit(abs_offset, 0, value);
}

int mio_get_mem16(uint16_t abs_offset)
{
    return mio_get_word(abs_offset);
}

int mio_set_mem16(uint16_t abs_offset, uint16_t value)
{
    return mio_set_word(abs_offset, value);
}

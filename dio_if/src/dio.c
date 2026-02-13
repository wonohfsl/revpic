#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/ioctl.h>

#include "piControl.h"
#include "mio_addr.h"
#include "dio.h"

/* ----------------------------------------------------------
   Utility: timestamp in microseconds
   ---------------------------------------------------------- */
static long long now_us(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000000LL + ts.tv_nsec / 1000;
}

/* ----------------------------------------------------------
   Utility: read a single bit
   ---------------------------------------------------------- */
static int read_bit(int fd, int offset, int bit)
{
    SPIValue val;
    memset(&val, 0, sizeof(val));
    val.i16uAddress = offset;
    val.i8uBit      = bit;

    int ret = ioctl(fd, KB_GET_VALUE, &val);
    if (ret < 0) {
        printf("KB_GET_VALUE ERROR\n");
        return -1;
    }

    return val.i8uValue;
}

/* ----------------------------------------------------------
   Utility: write a single bit
   ---------------------------------------------------------- */
static int write_bit(int fd, int offset, int bit, int value)
{
    SPIValue val;
    memset(&val, 0, sizeof(val));
    val.i16uAddress = offset;
    val.i8uBit      = bit;
    val.i8uValue    = value;

    int ret = ioctl(fd, KB_SET_VALUE, &val);
    if (ret < 0) {
        printf("KB_SET_VALUE ERROR\n");
        return 0;
    }
    return 1;
}

/* ----------------------------------------------------------
   Channel → offset/bit mapping
   ---------------------------------------------------------- */
static int get_di_offset_bit(int ch, int *offset, int *bit)
{
    switch (ch) {
        case 1: *offset = DI1_OFFSET; *bit = DI1_BIT; return 1;
        case 2: *offset = DI2_OFFSET; *bit = DI2_BIT; return 1;
        case 3: *offset = DI3_OFFSET; *bit = DI3_BIT; return 1;
        case 4: *offset = DI4_OFFSET; *bit = DI4_BIT; return 1;
        default: return 0;
    }
}

static int get_do_offset_bit(int ch, int *offset, int *bit)
{
    switch (ch) {
        case 1: *offset = DO1_OFFSET; *bit = DO1_BIT; return 1;
        case 2: *offset = DO2_OFFSET; *bit = DO2_BIT; return 1;
        case 3: *offset = DO3_OFFSET; *bit = DO3_BIT; return 1;
        case 4: *offset = DO4_OFFSET; *bit = DO4_BIT; return 1;
        default: return 0;
    }
}

/* ----------------------------------------------------------
   PUBLIC: GetDigitalInput
   ---------------------------------------------------------- */
int dio_get(int fd, int channel, int *value)
{
    int offset, bit;

    if (!get_di_offset_bit(channel, &offset, &bit)) {
        printf("Invalid DI channel %d\n", channel);
        return 0;
    }

    int v = read_bit(fd, offset, bit);
    if (v < 0) return 0;

    *value = v;
    return 1;
}

/* ----------------------------------------------------------
   PUBLIC: SetDigitalOutput
   Includes wait-until DI mirror matches DO
   ---------------------------------------------------------- */
int dio_set(int fd, int channel, int value, long long timeout_us)
{
    int do_offset, do_bit;
    int di_offset, di_bit;

    if (!get_do_offset_bit(channel, &do_offset, &do_bit)) {
        printf("Invalid DO channel %d\n", channel);
        return 0;
    }

    /* Mirror DI channel is same number */
    if (!get_di_offset_bit(channel, &di_offset, &di_bit)) {
        printf("Invalid DI mirror for DO%d\n", channel);
        return 0;
    }

    /* Write DO */
    if (!write_bit(fd, do_offset, do_bit, value))
        return 0;

    /* Wait for DI mirror */
    long long t0 = now_us();
    long long elapsed;

    while (1) {
        int di_val = read_bit(fd, di_offset, di_bit);
        elapsed = now_us() - t0;

        if (di_val == value)
            return 1;

        if (elapsed >= timeout_us) {
            printf("TIMEOUT: DI%d did not become %d\n", channel, value);
            return 0;
        }
    }
}

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <time.h>        // <-- REQUIRED for clock_gettime()

#include "piControl.h"
#include "mio_addr.h"

static void print_ioctl_result(const char *label, int ret)
{
    if (ret < 0) {
        printf("%s: ERROR ret=%d errno=%d (%s)\n",
               label, ret, errno, strerror(errno));
    }
}

static long long now_us(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000000LL + ts.tv_nsec / 1000;
}

static int read_bit(int fd, int offset, int bit)
{
    SPIValue val;
    memset(&val, 0, sizeof(val));
    val.i16uAddress = offset;
    val.i8uBit      = bit;

    int ret = ioctl(fd, KB_GET_VALUE, &val);
    print_ioctl_result("KB_GET_VALUE", ret);

    return (ret < 0) ? -1 : val.i8uValue;
}

static void write_bit(int fd, int offset, int bit, int value)
{
    SPIValue val;
    memset(&val, 0, sizeof(val));
    val.i16uAddress = offset;
    val.i8uBit      = bit;
    val.i8uValue    = value;

    int ret = ioctl(fd, KB_SET_VALUE, &val);
    print_ioctl_result("KB_SET_VALUE", ret);
}

int main(void)
{
    printf("Opening /dev/piControl0...\n");
    int fd = open("/dev/piControl0", O_RDWR);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    printf("\n=== TEST: DI1 <-> DO3 ===\n");

    int di1 = 0, di3 = 0, do3=0;
    long long t0 = 0;
    long long timeout = 50000;   // 50 ms
    long long elapsed = 0;

    /* 1. Read DI1 initial */
    di1 = read_bit(fd, DI1_OFFSET, DI1_BIT);
    printf("DI1 = %d\n", di1);

    /* 2. Write DO3 = 1 */
	do3 = 1;
    write_bit(fd, DO3_OFFSET, DO3_BIT, do3);
    printf("DO3 <-- %d\n", do3);

    /* 2a. Wait until DI3 becomes 1 or timeout */
    t0 = now_us();
    while (1) {
        di3 = read_bit(fd, DI3_OFFSET, DI3_BIT);
        elapsed = now_us() - t0;

        if (di3 == do3) {
            printf("DI3 = %d after %lld us\n", di3, elapsed);
            break;
        }

        if (elapsed >= timeout) {
            printf("WARNING(TIMEOUT): DI3 did not become %d within %lld us\n", do3, timeout);
            break;
        }
    }

    /* 3. Read DI1 */
    di1 = read_bit(fd, DI1_OFFSET, DI1_BIT);
    printf("DI1 = %d\n", di1);

    /* 2. Write DO3 = 0 */
	do3 = 0;
    write_bit(fd, DO3_OFFSET, DO3_BIT, do3);
    printf("DO3 <-- %d\n", do3);

    /* 2a. Wait until DI3 becomes 0 or timeout */
    t0 = now_us();
    while (1) {
        di3 = read_bit(fd, DI3_OFFSET, DI3_BIT);
        elapsed = now_us() - t0;

        if (di3 == do3) {
            printf("DI3 = %d after %lld us\n", di3, elapsed);
            break;
        }

        if (elapsed >= timeout) {
            printf("WARNING(TIMEOUT): DI3 did not become %d within %lld us\n", do3, timeout);
            break;
        }
    }

    /* 5. Read DI1 */
    di1 = read_bit(fd, DI1_OFFSET, DI1_BIT);
    printf("DI1 = %d\n", di1);

    close(fd);
    return 0;
}

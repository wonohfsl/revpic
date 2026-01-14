#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <time.h>

#include "piControl.h"
#include "mio_addr.h"

static void print_ioctl_result(const char *label, int ret)
{
    if (ret < 0) {
        printf("%s: ERROR ret=%d errno=%d (%s)\n",
               label, ret, errno, strerror(errno));
    }
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

static long long now_us(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000000LL + ts.tv_nsec / 1000;
}

int main(void)
{
    printf("Opening /dev/piControl0...\n");
    int fd = open("/dev/piControl0", O_RDWR);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    printf("\n=== TEST: DI1 <-> DO3 (timed) ===\n");

    /* 1. Read DI1 initial */
    int di1 = read_bit(fd, DI1_OFFSET, DI1_BIT);
    printf("DI1 initial = %d\n", di1);

    /* ============================================================
       2. WRITE DO3 = 1 AND WAIT UNTIL DI1 BECOMES 1
       ============================================================ */
    write_bit(fd, DO3_OFFSET, DO3_BIT, 1);
    printf("DO3 <-- 1\n");

    long long t0 = now_us();
    while (1) {
        di1 = read_bit(fd, DI1_OFFSET, DI1_BIT);
        if (di1 == 1)
            break;
    }
    long long t1 = now_us();
    printf("DI1 became 1 after %lld us\n", (t1 - t0));

    /* ============================================================
       3. WRITE DO3 = 0 AND WAIT UNTIL DI1 BECOMES 0
       ============================================================ */
    write_bit(fd, DO3_OFFSET, DO3_BIT, 0);
    printf("DO3 <-- 0\n");

    t0 = now_us();
    while (1) {
        di1 = read_bit(fd, DI1_OFFSET, DI1_BIT);
        if (di1 == 0)
            break;
    }
    t1 = now_us();
    printf("DI1 became 0 after %lld us\n", (t1 - t0));

    close(fd);
    return 0;
}
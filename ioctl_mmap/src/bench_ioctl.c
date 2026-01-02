#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include "piControl.h"

#define DI_BYTE_OFFSET 0
#define DI1_BIT        0
#define ITERATIONS     100000

int main(void)
{
    int fd = open("/dev/piControl0", O_RDWR);
    if (fd < 0) {
        perror("open /dev/piControl0");
        return 1;
    }

    SPIValue val;
    val.i16uAddress = DI_BYTE_OFFSET;
    val.i8uBit      = DI1_BIT;

    struct timeval start, end;

    gettimeofday(&start, NULL);

    for (int i = 0; i < ITERATIONS; i++) {
        if (ioctl(fd, KB_GET_VALUE, &val) < 0) {
            perror("ioctl(KB_GET_VALUE)");
            close(fd);
            return 1;
        }

        int di1 = val.i8uValue;   // 0 or 1
        (void)di1;                // suppress unused warning
    }

    gettimeofday(&end, NULL);

    long total_us =
        (end.tv_sec - start.tv_sec) * 1000000L +
        (end.tv_usec - start.tv_usec);

    double per_read = (double)total_us / (double)ITERATIONS;

    printf("IOCTL benchmark:\n");
    printf("  Total time: %ld us\n", total_us);
    printf("  Per read:   %.3f us\n", per_read);

    close(fd);
    return 0;
}

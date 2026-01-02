#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include "piControl.h"

#define DI_BYTE_OFFSET 0
#define DI1_BIT        0
#define MAP_SIZE       4096

int main(void)
{
    int fd = open("/dev/piControl0", O_RDWR | O_SYNC);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    uint8_t *pi = mmap(NULL, MAP_SIZE,
                       PROT_READ | PROT_WRITE,
                       MAP_SHARED, fd, 0);

    if (pi == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    struct timeval start, end;

    gettimeofday(&start, NULL);

    for (int i = 0; i < 100000; i++) {
        uint8_t byte = pi[DI_BYTE_OFFSET];
        int di1 = (byte >> DI1_BIT) & 1;
        (void)di1;
    }

    gettimeofday(&end, NULL);

    long us =
        (end.tv_sec - start.tv_sec) * 1000000L +
        (end.tv_usec - start.tv_usec);

    printf("MMAP:  total = %ld us, per read = %.2f us\n",
           us, (double)us / 100000.0);

    munmap(pi, MAP_SIZE);
    close(fd);
    return 0;
}

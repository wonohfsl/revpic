#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
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

    while (1) {
        uint8_t byte = pi[DI_BYTE_OFFSET];
        int di1 = (byte >> DI1_BIT) & 0x01;

        printf("MMAP:  DI1 = %d\n", di1);
        sleep(1);
    }

    munmap(pi, MAP_SIZE);
    close(fd);
    return 0;
}

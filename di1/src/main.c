#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include <errno.h>

#include "piControl.h"

int main() {
    int fd = open("/dev/piControl0", O_RDWR);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    SPIValue val;
    memset(&val, 0, sizeof(val));

    val.i16uAddress = 0;   // offset for DigitalInput_1
    val.i8uBit = 8;        // >= 8 means "read whole byte"

    if (ioctl(fd, KB_GET_VALUE, &val) < 0) {
        perror("ioctl(KB_GET_VALUE)");
        close(fd);
        return 1;
    }

    printf("DigitalInput_1 = %u\n", val.i8uValue);

    close(fd);
    return 0;
}

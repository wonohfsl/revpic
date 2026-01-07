#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "piControl.h"

int main(void)
{
    int fd = open("/dev/piControl0", O_RDWR);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    SPIValue v;

    /* -----------------------------
       1. Read DI1 (offset 13, bit 0)
       ----------------------------- */
    v.i16uAddress = 13;
    v.i8uBit      = 0;
    ioctl(fd, KB_GET_VALUE, &v);
    printf("DI1 = %d\n", v.i8uValue);

    /* -----------------------------
       2. Write DO3 = 1 (offset 47, bit 2)
       ----------------------------- */
    v.i16uAddress = 47;
    v.i8uBit      = 2;
    v.i8uValue    = 1;
    ioctl(fd, KB_SET_VALUE, &v);
    printf("Set DO3 to 1\n");

    /* -----------------------------
       3. Read DI1 again
       ----------------------------- */
    v.i16uAddress = 13;
    v.i8uBit      = 0;
    ioctl(fd, KB_GET_VALUE, &v);
    printf("DI1 = %d\n", v.i8uValue);

    /* -----------------------------
       4. Write DO3 = 0
       ----------------------------- */
    v.i16uAddress = 47;
    v.i8uBit      = 2;
    v.i8uValue    = 0;
    ioctl(fd, KB_SET_VALUE, &v);
    printf("Reset DO3 to 0\n");

    /* -----------------------------
       5. Read DI1 again
       ----------------------------- */
    v.i16uAddress = 13;
    v.i8uBit      = 0;
    ioctl(fd, KB_GET_VALUE, &v);
    printf("DI1 = %d\n", v.i8uValue);

    close(fd);
    return 0;
}

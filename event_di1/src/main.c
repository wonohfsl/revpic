#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <time.h>
#include "piControl.h"

#define DI_BYTE_OFFSET   0
#define DI1_BIT          0

#define DO_BYTE_OFFSET   0
#define DO3_BIT          2   // DO3 = bit 2

int main(void)
{
    int fd = open("/dev/piControl0", O_RDWR);
    if (fd < 0) {
        perror("open /dev/piControl0");
        return 1;
    }

    SPIValue di;
    di.i16uAddress = DI_BYTE_OFFSET;
    di.i8uBit      = DI1_BIT;

    SPIValue do3;
    do3.i16uAddress = DO_BYTE_OFFSET;
    do3.i8uBit      = DO3_BIT;

    int last = -1;

    while (1) {
        // Read DI1
        if (ioctl(fd, KB_GET_VALUE, &di) < 0) {
            perror("KB_GET_VALUE");
            return 1;
        }

        int now = di.i8uValue;

        // Rising edge detection: 0 -> 1
        if (last == 0 && now == 1) {

            // Write DO3 = 0
            do3.i8uValue = 0;
            if (ioctl(fd, KB_SET_VALUE, &do3) < 0) {
                perror("KB_SET_VALUE");
                return 1;
            }

            // Read DI1 again (confirmation)
            ioctl(fd, KB_GET_VALUE, &di);
            int confirm = di.i8uValue;

            // Get event timestamp
            struct timespec ts;
            clock_gettime(CLOCK_REALTIME, &ts);

            printf("EVENT: DI1 rose to 1 at %ld.%06ld sec, confirm DI1=%d\n",
                   ts.tv_sec, ts.tv_nsec / 1000, confirm);
        }

        last = now;

        // Small sleep to avoid 100% CPU
        usleep(1000);  // 1 ms
    }

    close(fd);
    return 0;
}

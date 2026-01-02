#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "piControl.h"

#define DI_BYTE_OFFSET 0
#define DI1_BIT        0

int main(void)
{
    int fd = open("/dev/piControl0", O_RDWR);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    //uint8_t buffer[1024];
	//SValue val; 
	SPIValue val; // <-- correct struct name
	val.i16uAddress = DI_BYTE_OFFSET; 
	val.i8uBit = DI1_BIT;

    while (1) {
        // Copy process image into buffer
//        if (ioctl(fd, KB_GET_PROCESS_IMAGE, buffer) < 0) {
//            perror("ioctl");
//            return 1;
//        }

		if (ioctl(fd, KB_GET_VALUE, &val) < 0) { 
			perror("ioctl(KB_GET_VALUE)"); 
			return 1; 
		}

        //uint8_t byte = buffer[DI_BYTE_OFFSET];
        //int di1 = (byte >> DI1_BIT) & 0x01;
		int di1 = val.i8uValue;

        printf("IOCTL: DI1 = %d\n", di1);
        sleep(1);
    }

    close(fd);
    return 0;
}

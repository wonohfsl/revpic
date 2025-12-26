#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include "piControl.h"

int main(void)
{
    int fd = open(PICONTROL_DEVICE, O_RDWR);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    printf("=== Read AnalogInput_1 ===\n");

    /* ---------------------------------------------------------
     * 1. Read using PiCtory variable name (AnalogInput_1)
     * --------------------------------------------------------- */
    SPIVariable var;
    memset(&var, 0, sizeof(var));
    strncpy(var.strVarName, "AnalogInput_1", sizeof(var.strVarName)-1);

    if (ioctl(fd, KB_FIND_VARIABLE, &var) < 0) {
        perror("KB_FIND_VARIABLE");
        close(fd);
        return 1;
    }

    SPIValue val1;
    memset(&val1, 0, sizeof(val1));
    val1.i16uAddress = var.i16uAddress;   // e.g., 18
    val1.i8uBit      = var.i8uBit;        // always 0 for analog
                                          
    if (ioctl(fd, KB_GET_VALUE, &val1) < 0) {
        perror("KB_GET_VALUE");
        close(fd);
        return 1;
    }

    /* Analog values are 16-bit, so read two bytes */
    printf("Method 1 (variable name): AnalogInput_1 = %u\n", val1.i8uValue);


    /* ---------------------------------------------------------
     * 2. Read using raw offset + bit
     *    From your piTest output:
     *    AnalogInput_1 → offset = 18, bit = 0, length = 16 bits
     * --------------------------------------------------------- */
    SPIValue val2;
    memset(&val2, 0, sizeof(val2));
    val2.i16uAddress = 18;   // offset for AI1
    val2.i8uBit      = 0;    // always 0 for analog

    if (ioctl(fd, KB_GET_VALUE, &val2) < 0) {
        perror("KB_GET_VALUE");
        close(fd);
        return 1;
    }

    printf("Method 2 (offset+bit):    AnalogInput_1 = %u\n", val2.i8uValue);

    close(fd);
    return 0;
}

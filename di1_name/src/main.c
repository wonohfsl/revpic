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

    printf("=== Read DigitalInput_1 ===\n");

    /* ---------------------------------------------------------
     * 1. Read using PiCtory variable name (DigitalInput_1)
     * --------------------------------------------------------- */
    SPIVariable var;
    memset(&var, 0, sizeof(var));
    strncpy(var.strVarName, "DigitalInput_1", sizeof(var.strVarName)-1);

    if (ioctl(fd, KB_FIND_VARIABLE, &var) < 0) {
        perror("KB_FIND_VARIABLE");
        close(fd);
        return 1;
    }

    SPIValue val1;
    memset(&val1, 0, sizeof(val1));
    val1.i16uAddress = var.i16uAddress;
    val1.i8uBit      = var.i8uBit;

    if (ioctl(fd, KB_GET_VALUE, &val1) < 0) {
        perror("KB_GET_VALUE");
        close(fd);
        return 1;
    }

    printf("Method 1 (variable name): DigitalInput_1 = %d\n", val1.i8uValue);


    /* ---------------------------------------------------------
     * 2. Read using raw offset + bit
     *    From your piTest output:
     *    DigitalInput_1 → offset = 0, bit = 0
     * --------------------------------------------------------- */
    SPIValue val2;
    memset(&val2, 0, sizeof(val2));
    val2.i16uAddress = 0;   // offset
    val2.i8uBit      = 0;   // bit

    if (ioctl(fd, KB_GET_VALUE, &val2) < 0) {
        perror("KB_GET_VALUE");
        close(fd);
        return 1;
    }

    printf("Method 2 (offset+bit):    DigitalInput_1 = %d\n", val2.i8uValue);

    close(fd);
    return 0;
}

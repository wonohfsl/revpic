#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "piControl.h"

/*
 * This program executes continuously until DigitalInput_1 transitions to a value of 1.
 * To run the program in the background, use:   ./myapp &
 * To terminate the background process, use:    killall myapp
 */

int main(void)
{
    int fd = open("/dev/piControl0", O_RDWR | O_NONBLOCK);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    /* Find DI1 once */
    SPIVariable var;
    memset(&var, 0, sizeof(var));
    strncpy(var.strVarName, "DigitalInput_1", sizeof(var.strVarName)-1);

    if (ioctl(fd, KB_FIND_VARIABLE, &var) < 0) {
        perror("KB_FIND_VARIABLE");
        close(fd);
        return 1;
    }

    /* Prepare read struct */
    SPIValue val;
    memset(&val, 0, sizeof(val));
    val.i16uAddress = var.i16uAddress;
    val.i8uBit      = var.i8uBit;

    /* Poll silently */
    while (1) {
        if (ioctl(fd, KB_GET_VALUE, &val) == 0) {
            if (val.i8uValue == 1) {
                printf("DI1 is now 1 — exiting.\n");
                break;
            }
        }
        sleep(1);
    }

    close(fd);
    return 0;
}

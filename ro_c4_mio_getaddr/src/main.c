#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include "piControl.h"

static void print_ioctl_result(const char *label, int ret)
{
    if (ret < 0) {
        printf("%s: ERROR ret=%d errno=%d (%s)\n",
               label, ret, errno, strerror(errno));
    }
}

static int find_and_print(int fd, const char *name)
{
    SPIVariable var;
    memset(&var, 0, sizeof(var));
    strncpy(var.strVarName, name, sizeof(var.strVarName)-1);

    int ret = ioctl(fd, KB_FIND_VARIABLE, &var);
    print_ioctl_result(name, ret);

    if (ret >= 0) {
        printf("%s: offset=%d bit=%d len=%d\n",
               name, var.i16uAddress, var.i8uBit, var.i16uLength);
    }

    return ret;
}

int main(void)
{
    int fd;

    printf("Opening /dev/piControl0...\n");
    fd = open("/dev/piControl0", O_RDWR);
    if (fd < 0) {
        perror("open");
        return 1;
    }

	printf("\n====== MIO Module ======\n");
    printf("\n=== Digital Inputs ===\n");
    find_and_print(fd, "DigitalInput_1");
    find_and_print(fd, "DigitalInput_2");
    find_and_print(fd, "DigitalInput_3");
    find_and_print(fd, "DigitalInput_4");

    printf("\n=== Digital Outputs ===\n");
    find_and_print(fd, "DigitalOutput_1");
    find_and_print(fd, "DigitalOutput_2");
    find_and_print(fd, "DigitalOutput_3");
    find_and_print(fd, "DigitalOutput_4");

    printf("\n=== Analog Inputs ===\n");
    find_and_print(fd, "AnalogInput_1");
    find_and_print(fd, "AnalogInput_2");
    find_and_print(fd, "AnalogInput_3");
    find_and_print(fd, "AnalogInput_4");
    find_and_print(fd, "AnalogInput_5");
    find_and_print(fd, "AnalogInput_6");
    find_and_print(fd, "AnalogInput_7");
    find_and_print(fd, "AnalogInput_8");

    printf("\n=== Analog Outputs ===\n");
    find_and_print(fd, "AnalogOutput_1");
    find_and_print(fd, "AnalogOutput_2");
    find_and_print(fd, "AnalogOutput_3");
    find_and_print(fd, "AnalogOutput_4");
    find_and_print(fd, "AnalogOutput_5");
    find_and_print(fd, "AnalogOutput_6");
    find_and_print(fd, "AnalogOutput_7");
    find_and_print(fd, "AnalogOutput_8");

    printf("\n=== IO Modes ===\n");
    find_and_print(fd, "IO_Mode_1");
    find_and_print(fd, "IO_Mode_2");
    find_and_print(fd, "IO_Mode_3");
    find_and_print(fd, "IO_Mode_4");

	printf("\n====== RO Module ======\n");
    printf("\n=== Relay Outputs ===\n");
    find_and_print(fd, "RelayOutput_1");
    find_and_print(fd, "RelayOutput_2");
    find_and_print(fd, "RelayOutput_3");
    find_and_print(fd, "RelayOutput_4");

    close(fd);
    return 0;
}
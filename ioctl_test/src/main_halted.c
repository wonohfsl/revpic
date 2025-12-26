#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "piControl.h"

int main(void) {
    int fd = open("/dev/piControl0", O_RDWR);
    if (fd < 0) {
        perror("open /dev/piControl0");
        return 1;
    }

    printf("=== piControl ioctl test harness ===\n");

    // --- KB_GET_VALUE ---
    SPIValue val = {0};
    val.i16uAddress = 0;
    val.i8uBit = 0;
    if (ioctl(fd, KB_GET_VALUE, &val) == 0)
        printf("KB_GET_VALUE: offset=0 bit=0 value=%d\n", val.i8uValue);

    // --- KB_SET_VALUE ---
    val.i16uAddress = 34;
    val.i8uBit = 0;
    val.i8uValue = 1;
    if (ioctl(fd, KB_SET_VALUE, &val) == 0)
        printf("KB_SET_VALUE: offset=34 bit=0 set to 1\n");

    // --- KB_GET_DEVICE_INFO ---
    SDeviceInfo devInfo;
    if (ioctl(fd, KB_GET_DEVICE_INFO, &devInfo) == 0) {
        printf("KB_GET_DEVICE_INFO: type=%u serial=%u\n",
               devInfo.i16uModuleType,
               devInfo.i32uSerialnumber);  // note lowercase 'n'
    }

    // --- KB_FIND_VARIABLE ---
    SPIVariable var;
    memset(&var, 0, sizeof(var));
    strncpy(var.strVarName, "DigitalInput_1", sizeof(var.strVarName)-1);
    if (ioctl(fd, KB_FIND_VARIABLE, &var) == 0) {
        printf("KB_FIND_VARIABLE: %s offset=%u len=%u bit=%u\n",
               var.strVarName, var.i16uAddress, var.i16uLength, var.i8uBit);
    }

    // --- KB_DIO_RESET_COUNTER ---
    SDIOResetCounter dioReset;
    dioReset.i8uAddress = 34; // correct field name
    if (ioctl(fd, KB_DIO_RESET_COUNTER, &dioReset) == 0)
        printf("KB_DIO_RESET_COUNTER: reset counter at offset 34\n");

    // --- KB_GET_LAST_MESSAGE ---
    int lastMsg = 0;
    if (ioctl(fd, KB_GET_LAST_MESSAGE, &lastMsg) == 0)
        printf("KB_GET_LAST_MESSAGE: %d\n", lastMsg);

    // --- KB_RO_GET_COUNTER ---
    int counterVal = 0;
    if (ioctl(fd, KB_RO_GET_COUNTER, &counterVal) == 0)
        printf("KB_RO_GET_COUNTER: %d\n", counterVal);

    // --- KB_RESET ---
    if (ioctl(fd, KB_RESET, NULL) == 0)
        printf("KB_RESET: driver reset\n");

    // --- KB_STOP_IO ---
    if (ioctl(fd, KB_STOP_IO, NULL) == 0)
        printf("KB_STOP_IO: stopped IO\n");

    // --- KB_CONFIG_STOP / SEND / START ---
    SConfigData cfg;
    memset(&cfg, 0, sizeof(cfg));
    ioctl(fd, KB_CONFIG_STOP, &cfg);
    ioctl(fd, KB_CONFIG_SEND, &cfg);
    ioctl(fd, KB_CONFIG_START, &cfg);
    printf("KB_CONFIG_* sequence executed\n");

    // --- KB_SET_OUTPUT_WATCHDOG ---
    int watchdog = 1000; // ms
    ioctl(fd, KB_SET_OUTPUT_WATCHDOG, &watchdog);
    printf("KB_SET_OUTPUT_WATCHDOG: %d ms\n", watchdog);

    // --- KB_SET_POS ---
    int pos = 0;
    ioctl(fd, KB_SET_POS, &pos);
    printf("KB_SET_POS: %d\n", pos);

    // --- KB_AIO_CALIBRATE ---
    ioctl(fd, KB_AIO_CALIBRATE, NULL);
    printf("KB_AIO_CALIBRATE executed\n");

    // --- KB_WAIT_FOR_EVENT ---
    ioctl(fd, KB_WAIT_FOR_EVENT, NULL);
    printf("KB_WAIT_FOR_EVENT executed\n");

    close(fd);
    return 0;
}

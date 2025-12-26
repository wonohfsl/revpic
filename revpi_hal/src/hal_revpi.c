#include "hal.h"
#include "../../../piControl/src/piControl.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include <stdio.h>

static int fd = -1;

int hal_revpi_init(void) {
    fd = open("/dev/piControl0", O_RDWR);
    return (fd >= 0) ? 0 : -1;
}

void hal_revpi_close(void) {
    if (fd >= 0) close(fd);
}

uint8_t hal_revpi_read_byte(uint16_t offset) {
    SPIValue val = {0};
    val.i16uAddress = offset;
    val.i8uBit = 8;
    if (ioctl(fd, KB_GET_VALUE, &val) < 0) return 0;
    return val.i8uValue;
}

void hal_revpi_write_byte(uint16_t offset, uint8_t value) {
    SPIValue val = {0};
    val.i16uAddress = offset;
    val.i8uBit = 8;
    val.i8uValue = value;
    ioctl(fd, KB_SET_VALUE, &val);
}

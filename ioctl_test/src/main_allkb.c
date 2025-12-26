#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <signal.h>
#include <errno.h>
#include "piControl.h"   // your local header in src/

static void print_errno(const char *label) {
    fprintf(stderr, "%s failed: errno=%d (%s)\n", label, errno, strerror(errno));
}

static int ioctl_try(int fd, unsigned long req, void *arg, const char *label) {
    int rc = ioctl(fd, req, arg);
    if (rc == 0) {
        printf("%s: OK\n", label);
    } else {
        print_errno(label);
    }
    return rc;
}

static volatile sig_atomic_t alarm_fired = 0;
static void alarm_handler(int sig) {
    (void)sig;
    alarm_fired = 1;
}

int main(void) {
    // Open device in non-blocking mode
    int fd = open("/dev/piControl0", O_RDWR | O_NONBLOCK);
    if (fd < 0) {
        perror("open /dev/piControl0");
        return 1;
    }

    printf("=== piControl ioctl test harness ===\n");

    // --- KB_GET_VALUE ---
    SPIValue val;
    memset(&val, 0, sizeof(val));
    val.i16uAddress = 0;
    val.i8uBit = 0;
    if (ioctl(fd, KB_GET_VALUE, &val) == 0)
        printf("KB_GET_VALUE: offset=0 bit=0 value=%d\n", val.i8uValue);
    else
        print_errno("KB_GET_VALUE");

    // --- KB_SET_VALUE ---
    val.i16uAddress = 34;
    val.i8uBit = 0;
    val.i8uValue = 1;
    if (ioctl(fd, KB_SET_VALUE, &val) == 0)
        printf("KB_SET_VALUE: offset=34 bit=0 set to 1\n");
    else
        print_errno("KB_SET_VALUE");

    // --- KB_GET_DEVICE_INFO ---
    SDeviceInfo devInfo;
    if (ioctl(fd, KB_GET_DEVICE_INFO, &devInfo) == 0) {
        printf("KB_GET_DEVICE_INFO: type=%u serial=%u\n",
               devInfo.i16uModuleType, devInfo.i32uSerialnumber);
    } else {
        print_errno("KB_GET_DEVICE_INFO");
    }

    // --- KB_FIND_VARIABLE ---
    SPIVariable var;
    memset(&var, 0, sizeof(var));
    strncpy(var.strVarName, "DigitalInput_1", sizeof(var.strVarName)-1);
    if (ioctl(fd, KB_FIND_VARIABLE, &var) == 0) {
        printf("KB_FIND_VARIABLE: %s offset=%u len=%u bit=%u\n",
               var.strVarName, var.i16uAddress, var.i16uLength, var.i8uBit);
    } else {
        print_errno("KB_FIND_VARIABLE");
    }

    // --- KB_DIO_RESET_COUNTER ---
    SDIOResetCounter dioReset;
    memset(&dioReset, 0, sizeof(dioReset));
    dioReset.i8uAddress = 34; // example DO offset
    if (ioctl(fd, KB_DIO_RESET_COUNTER, &dioReset) == 0)
        printf("KB_DIO_RESET_COUNTER: reset counter at offset 34\n");
    else
        print_errno("KB_DIO_RESET_COUNTER");

    // --- KB_GET_LAST_MESSAGE ---
    int lastMsg = 0;
    if (ioctl(fd, KB_GET_LAST_MESSAGE, &lastMsg) == 0)
        printf("KB_GET_LAST_MESSAGE: %d\n", lastMsg);
    else
        print_errno("KB_GET_LAST_MESSAGE");

    // --- KB_RO_GET_COUNTER ---
    int counterVal = 0;
    if (ioctl(fd, KB_RO_GET_COUNTER, &counterVal) == 0)
        printf("KB_RO_GET_COUNTER: %d\n", counterVal);
    else
        print_errno("KB_RO_GET_COUNTER");

    // --- KB_RESET ---
    if (ioctl(fd, KB_RESET, NULL) == 0)
        printf("KB_RESET: driver reset\n");
    else
        print_errno("KB_RESET");

    // --- KB_STOP_IO ---
    if (ioctl(fd, KB_STOP_IO, NULL) == 0)
        printf("KB_STOP_IO: stopped IO\n");
    else
        print_errno("KB_STOP_IO");

    // --- KB_CONFIG_STOP / SEND / START ---
    SConfigData cfg;
    memset(&cfg, 0, sizeof(cfg));
    ioctl_try(fd, KB_CONFIG_STOP, &cfg, "KB_CONFIG_STOP");
    ioctl_try(fd, KB_CONFIG_SEND, &cfg, "KB_CONFIG_SEND");
    ioctl_try(fd, KB_CONFIG_START, &cfg, "KB_CONFIG_START");
    printf("KB_CONFIG_* sequence executed\n");

    // --- KB_SET_OUTPUT_WATCHDOG ---
    int watchdog = 1000; // ms
    if (ioctl(fd, KB_SET_OUTPUT_WATCHDOG, &watchdog) == 0)
        printf("KB_SET_OUTPUT_WATCHDOG: %d ms\n", watchdog);
    else
        print_errno("KB_SET_OUTPUT_WATCHDOG");

    // --- KB_SET_POS ---
    int pos = 0;
    if (ioctl(fd, KB_SET_POS, &pos) == 0)
        printf("KB_SET_POS: %d\n", pos);
    else
        print_errno("KB_SET_POS");

    // --- KB_AIO_CALIBRATE with timeout ---
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = alarm_handler;
    sigaction(SIGALRM, &sa, NULL);
    alarm_fired = 0;
    alarm(2); // 2-second timeout

    errno = 0;
    int rc = ioctl(fd, KB_AIO_CALIBRATE, NULL);
    alarm(0);

    if (rc == 0) {
        printf("KB_AIO_CALIBRATE executed\n");
    } else if (errno == EINTR || alarm_fired) {
        printf("KB_AIO_CALIBRATE: timed out (skipping)\n");
    } else {
        print_errno("KB_AIO_CALIBRATE");
    }

    // --- KB_WAIT_FOR_EVENT (non-blocking with timeout) ---
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(fd, &fds);
    struct timeval tv = { .tv_sec = 1, .tv_usec = 0 }; // 1s timeout
    int sel = select(fd + 1, &fds, NULL, NULL, &tv);
    if (sel > 0) {
        if (ioctl(fd, KB_WAIT_FOR_EVENT, NULL) == 0)
            printf("KB_WAIT_FOR_EVENT: event received\n");
        else
            print_errno("KB_WAIT_FOR_EVENT");
    } else if (sel == 0) {
        printf("KB_WAIT_FOR_EVENT: timeout, no event\n");
    } else {
        print_errno("select");
    }

    close(fd);
    return 0;
}

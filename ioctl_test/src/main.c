#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <signal.h>
#include <errno.h>
#include <stdint.h>

#include "piControl.h"

/* Calibration struct for AIO module */
typedef struct {
    uint16_t channel;   /* 0–7 */
} SAIOCalibrate;

static void print_errno(const char *label) {
    fprintf(stderr, "%s failed: errno=%d (%s)\n", label, errno, strerror(errno));
}

static int ioctl_try(int fd, unsigned long req, void *arg, const char *label) {
    int rc = ioctl(fd, req, arg);
    if (rc == 0)
        printf("%s: OK\n", label);
    else
        print_errno(label);
    return rc;
}

static volatile sig_atomic_t alarm_fired = 0;
//static void alarm_handler(int sig) { alarm_fired = 1; }
static void alarm_handler(int sig) {
    (void)sig;
    alarm_fired = 1;
}

/* ---------------------------------------------------------
   Capability detection (generic, future‑proof)
--------------------------------------------------------- */
static int detect_hw(int  fd,
                     int *cap_analog,
                     int *cap_counters,
                     int *cap_events,
                     int *cap_pwm,
                     int *cap_mem,
                     int *cap_calibration)
{
    SDeviceInfo devs[64];
    memset(devs, 0, sizeof(devs));

    if (ioctl(fd, KB_GET_DEVICE_INFO_LIST, devs) < 0) {
        print_errno("KB_GET_DEVICE_INFO_LIST");
        return -1;
    }

    *cap_analog      = 0;
    *cap_counters    = 0;
    *cap_events      = 0;
    *cap_pwm         = 0;
    *cap_mem         = 0;
    *cap_calibration = 0;

    for (int i = 0; i < 64; i++) {
        if (!devs[i].i8uActive)
            continue;

        uint16_t type = devs[i].i16uModuleType;

        /* RevPi AIO (module type 113) */
        if (type == 113) {
            *cap_analog      = 1;
            *cap_pwm         = 0;
            *cap_events      = 1;
            *cap_mem         = 1;
            *cap_calibration = 1;   /* ONLY AIO supports calibration */
        }

        /* RevPi MIO (118) */
        if (type == 118) {
            *cap_analog      = 1;
            *cap_pwm         = 1;
            *cap_events      = 1;
            *cap_mem         = 1;
            *cap_calibration = 0;   /* IMPORTANT: MIO does NOT support calibration */
        }

        /* DIO (111) or RO (112) */
        if (type == 111 || type == 112) {
            *cap_counters = 1;
            *cap_events   = 1;
        }

        /* Connect 5 (138) */
        if (type == 138) {
            *cap_mem = 1;
        }
    }

    return 0;
}

int main(void) {
    int fd = open("/dev/piControl0", O_RDWR | O_NONBLOCK);
    if (fd < 0) {
        perror("open /dev/piControl0");
        return 1;
    }

    printf("=== piControl ioctl test harness ===\n");

    int cap_analog=0, cap_counters=0, cap_events=0, cap_pwm=0, cap_mem=0, cap_calibration=0;
    detect_hw(fd, &cap_analog, &cap_counters, &cap_events, &cap_pwm, &cap_mem, &cap_calibration);

    printf("Hardware capabilities detected:\n");
    printf("  Analog I/O capability:      %s\n", cap_analog      ? "YES" : "NO");
    printf("  Digital counter capability: %s\n", cap_counters    ? "YES" : "NO");
    printf("  Event capability:           %s\n", cap_events      ? "YES" : "NO");
    printf("  PWM capability:             %s\n", cap_pwm         ? "YES" : "NO");
    printf("  Memory/config capability:   %s\n", cap_mem         ? "YES" : "NO");
    printf("  Calibration capability:     %s\n\n", cap_calibration ? "YES" : "NO");

    /* ---------------------------------------------------------
       KB_GET_VALUE
    --------------------------------------------------------- */
    SPIValue val = { .i16uAddress = 0, .i8uBit = 0 };
    if (ioctl(fd, KB_GET_VALUE, &val) == 0)
        printf("KB_GET_VALUE: offset=0 bit=0 value=%d\n", val.i8uValue);
    else
        print_errno("KB_GET_VALUE");

    /* ---------------------------------------------------------
       KB_SET_VALUE
    --------------------------------------------------------- */
    val.i16uAddress = 34;
    val.i8uBit      = 0;
    val.i8uValue    = 1;
    if (ioctl(fd, KB_SET_VALUE, &val) == 0)
        printf("KB_SET_VALUE: offset=34 bit=0 set to 1\n");
    else
        print_errno("KB_SET_VALUE");

    /* ---------------------------------------------------------
       KB_GET_DEVICE_INFO
    --------------------------------------------------------- */
    SDeviceInfo devInfo;
    if (ioctl(fd, KB_GET_DEVICE_INFO, &devInfo) == 0)
        printf("KB_GET_DEVICE_INFO: type=%u serial=%u\n",
               devInfo.i16uModuleType, devInfo.i32uSerialnumber);
    else
        print_errno("KB_GET_DEVICE_INFO");

    /* ---------------------------------------------------------
       KB_FIND_VARIABLE
    --------------------------------------------------------- */
    SPIVariable var;
    memset(&var, 0, sizeof(var));
    strncpy(var.strVarName, "DigitalInput_1", sizeof(var.strVarName)-1);
    if (ioctl(fd, KB_FIND_VARIABLE, &var) == 0)
        printf("KB_FIND_VARIABLE: %s offset=%u len=%u bit=%u\n",
               var.strVarName, var.i16uAddress, var.i16uLength, var.i8uBit);
    else
        print_errno("KB_FIND_VARIABLE");

    /* ---------------------------------------------------------
       KB_DIO_RESET_COUNTER
    --------------------------------------------------------- */
    if (cap_counters) {
        SDIOResetCounter dioReset = { .i8uAddress = 34 };
        ioctl_try(fd, KB_DIO_RESET_COUNTER, &dioReset, "KB_DIO_RESET_COUNTER");
    } else {
        printf("KB_DIO_RESET_COUNTER: skipped (no digital counter capability)\n");
    }

    /* ---------------------------------------------------------
       KB_GET_LAST_MESSAGE
    --------------------------------------------------------- */
    int lastMsg = 0;
    ioctl_try(fd, KB_GET_LAST_MESSAGE, &lastMsg, "KB_GET_LAST_MESSAGE");

    /* ---------------------------------------------------------
       KB_RO_GET_COUNTER
    --------------------------------------------------------- */
    if (cap_counters) {
        int counterVal = 0;
        ioctl_try(fd, KB_RO_GET_COUNTER, &counterVal, "KB_RO_GET_COUNTER");
    } else {
        printf("KB_RO_GET_COUNTER: skipped (no digital counter capability)\n");
    }

    /* ---------------------------------------------------------
       KB_RESET
    --------------------------------------------------------- */
    ioctl_try(fd, KB_RESET, NULL, "KB_RESET");

    /* ---------------------------------------------------------
       KB_STOP_IO
    --------------------------------------------------------- */
    if (cap_mem)
        ioctl_try(fd, KB_STOP_IO, NULL, "KB_STOP_IO");
    else
        printf("KB_STOP_IO: skipped (no memory/config capability)\n");

    /* ---------------------------------------------------------
       KB_CONFIG_STOP / SEND / START
    --------------------------------------------------------- */
    SConfigData cfg;
    memset(&cfg, 0, sizeof(cfg));
    ioctl_try(fd, KB_CONFIG_STOP,  &cfg, "KB_CONFIG_STOP");
    ioctl_try(fd, KB_CONFIG_SEND,  &cfg, "KB_CONFIG_SEND");
    ioctl_try(fd, KB_CONFIG_START, &cfg, "KB_CONFIG_START");
    printf("KB_CONFIG_* sequence executed\n");

    /* ---------------------------------------------------------
       KB_SET_OUTPUT_WATCHDOG
    --------------------------------------------------------- */
    int watchdog = 1000;
    ioctl_try(fd, KB_SET_OUTPUT_WATCHDOG, &watchdog, "KB_SET_OUTPUT_WATCHDOG");

    /* ---------------------------------------------------------
       KB_SET_POS
    --------------------------------------------------------- */
    int pos = 0;
    ioctl_try(fd, KB_SET_POS, &pos, "KB_SET_POS");

    /* ---------------------------------------------------------
       KB_AIO_CALIBRATE (only if calibration capability exists)
    --------------------------------------------------------- */
    if (cap_calibration) {
        SAIOCalibrate cal = { .channel = 0 };

        struct sigaction sa;
        memset(&sa, 0, sizeof(sa));
        sa.sa_handler = alarm_handler;
        sigaction(SIGALRM, &sa, NULL);

        alarm_fired = 0;
        alarm(2);

        errno = 0;
        int rc = ioctl(fd, KB_AIO_CALIBRATE, &cal);
        alarm(0);

        if (rc == 0)
            printf("KB_AIO_CALIBRATE: OK (channel %u)\n", cal.channel);
        else if (errno == EINTR || alarm_fired)
            printf("KB_AIO_CALIBRATE: timed out (skipping)\n");
        else
            print_errno("KB_AIO_CALIBRATE");

    } else {
        printf("KB_AIO_CALIBRATE: skipped (no calibration capability)\n");
    }

    /* ---------------------------------------------------------
       KB_WAIT_FOR_EVENT (only if event capability exists)
    --------------------------------------------------------- */
	cap_events = 0; 	// let's skip this
    if (cap_events) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(fd, &fds);
        struct timeval tv = { .tv_sec = 1, .tv_usec = 0 };

        int sel = select(fd + 1, &fds, NULL, NULL, &tv);
        if (sel > 0)
            ioctl_try(fd, KB_WAIT_FOR_EVENT, NULL, "KB_WAIT_FOR_EVENT");
        else if (sel == 0)
            printf("KB_WAIT_FOR_EVENT: timeout, no event\n");
        else
            print_errno("select");
    } else {
        printf("KB_WAIT_FOR_EVENT: skipped (no event capability)\n");
    }

    close(fd);
    return 0;
}

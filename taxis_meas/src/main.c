#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>

#include "piControl.h"
#include "piControlIf.h"
#include "mio_addr.h"     // contains AI1_OFFSET

int main(void)
{
    if (piControlOpen() < 0) {
        printf("Cannot open piControl\n");
        return 1;
    }

    printf("Read T-axis position sensor value every second\n");
    printf("Time, Count, Value\n");

    uint16_t value = 0;
    int sample = 0;

    while (1) {
        // Read ADC
        if (piControlRead(AI1_OFFSET, 2, (uint8_t *)&value) < 0) {
            printf("Failed to read AI1\n");
            break;
        }

        // Get current time with milliseconds
        struct timeval tv;
        gettimeofday(&tv, NULL);
        struct tm *tm_info = localtime(&tv.tv_sec);
        int ms = tv.tv_usec / 1000;

        // Print in desired format
        printf("%02d:%02d:%02d.%03d, %d, %u\n",
               tm_info->tm_hour,
               tm_info->tm_min,
               tm_info->tm_sec,
               ms,
               sample,
               value);

        sample++;
        sleep(1);
    }

    piControlClose();
    return 0;
}

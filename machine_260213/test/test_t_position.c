/**
 * @file test_t_position.c
 * @brief T‑Axis Position Sensor Measurement Test (RevPi MIO AnalogInput_1)
 *
 * @details
 * This test program reads the raw ADC value from the T‑axis linear position
 * sensor connected to **RevPi MIO AnalogInput_1**. It prints timestamped
 * samples once per second for calibration, linearity analysis, and actuator
 * speed characterization.
 *
 * ## Hardware Context
 * - Device: **Revolution Pi MIO**
 * - Input: **AnalogInput_1 (AI1)**, 0–10 V, 16‑bit ADC
 * - Process Image: `AI1_OFFSET` (defined in `mio_addr.h`)
 * - Expected sensor output: 0–10 V proportional to actuator position
 *
 * ## Process Image Mapping
 * | Signal Name       | Size | Offset        | Description                     |
 * |-------------------|------|---------------|---------------------------------|
 * | AnalogInput_1     | 2 B  | AI1_OFFSET    | Raw ADC value (0–32767)         |
 *
 * ## Usage
 * Compile and run on RevPi OS:
 * @code
 *   gcc test_t_position.c -o test_t_position
 *   ./test_t_position
 * @endcode
 *
 * Output format:
 * @code
 *   HH:MM:SS.mmm, sample#, raw_value
 * @endcode
 *
 * Example:
 * @code
 *   14:32:10.123, 42, 15890
 * @endcode
 *
 * ## Notes
 * - Sampling rate is 1 Hz (adjustable).
 * - Raw ADC values can be converted to engineering units using your
 *   calibration mapping (e.g., inches or mm).
 * - Designed for actuator calibration, linearity plots, and speed tests.
 *
 * @author
 *   Wonoh / RevPi HAL Development
 */

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>

#include "piControl.h"
#include "piControlIf.h"
#include "mio_addr.h"     // contains AI1_OFFSET for AnalogInput_1


int main(void)
{
    /*--------------------------------------------------------------
     * Open piControl driver
     *--------------------------------------------------------------*/
    if (piControlOpen() < 0) {
        printf("Cannot open piControl\n");
        return 1;
    }

    printf("Read T-axis position sensor value every second\n");
    printf("Time, Count, Value\n");

    uint16_t value = 0;   // raw ADC value (0–32767 for 0–10 V input)
    int sample = 0;       // sample counter

    while (1) {

        /*----------------------------------------------------------
         * Read 2 bytes from AnalogInput_1
         *   - AI1_OFFSET is the byte offset in the process image
         *   - 2 bytes because analog inputs are 16-bit
         *----------------------------------------------------------*/
        if (piControlRead(AI1_OFFSET, 2, (uint8_t *)&value) < 0) {
            printf("Failed to read AI1\n");
            break;
        }

        /*----------------------------------------------------------
         * Get current timestamp with millisecond precision
         *----------------------------------------------------------*/
        struct timeval tv;
        gettimeofday(&tv, NULL);

        struct tm *tm_info = localtime(&tv.tv_sec);
        int ms = tv.tv_usec / 1000;

        /*----------------------------------------------------------
         * Print in CSV‑friendly format:
         *   HH:MM:SS.mmm, sample#, raw_value
         *----------------------------------------------------------*/
        printf("%02d:%02d:%02d.%03d, %d, %u\n",
               tm_info->tm_hour,
               tm_info->tm_min,
               tm_info->tm_sec,
               ms,
               sample,
               value);

        sample++;
        sleep(1);   // 1 Hz sampling rate
    }

    /*--------------------------------------------------------------
     * Close piControl driver
     *--------------------------------------------------------------*/
    piControlClose();
    return 0;
}

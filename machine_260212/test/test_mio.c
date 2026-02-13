#include <stdio.h>
#include <unistd.h>
#include <time.h>

#include "mio.h"

static long long now_us(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000000LL + ts.tv_nsec / 1000;
}

int main(void)
{
    if (mio_init() < 0) {
        printf("Failed to init MIO\n");
        return 1;
    }

    printf("=== MIO HAL TEST ===\n");

    long long t0 = 0;
    long long timeout = 200000;   // 200 ms
    long long elapsed = 0;
    int tolerance = 10;		// ±100 counts = ±0.1% of 0–10000 range

    int set_val=0, get_val=0;

    // === Digital Test ===

    printf("DI1 = %d\n\n", mio_get_di(1));

    // Set DO3 to 1

	set_val = 1;
	printf("DO3 <-- %d\n", set_val);
    mio_set_do(3, set_val);
    //usleep(50000);
    //printf("DO3 = %d\n", mio_get_do(3));

    t0 = now_us();
    while (1) {
        get_val = mio_get_di(3);
        elapsed = now_us() - t0;

        if (get_val == set_val) {
            printf("D03 = %d (after %lld us)\n", get_val, elapsed);
            break;
        }

        if (elapsed >= timeout) {
            printf("WARNING(TIMEOUT): DO3 did not become %d within %lld us\n", get_val, timeout);
            break;
        }
    }

	printf("DI1 = %d\n\n", mio_get_di(1));

	// Set DO3 to 0
	
	set_val = 0;
	printf("DO3 <-- %d\n", set_val);
    mio_set_do(3, set_val);
	//usleep(50000);
    //printf("DO3 = %d\n", mio_get_do(3));

    t0 = now_us();
    while (1) {
        get_val = mio_get_di(3);
        elapsed = now_us() - t0;

        if (get_val == set_val) {
            printf("D03 = %d (after %lld us)\n", get_val, elapsed);
            break;
        }

        if (elapsed >= timeout) {
            printf("WARNING(TIMEOUT): DO3 did not become %d within %lld us\n", get_val, timeout);
            break;
        }
    }

    printf("DI1 = %d\n\n", mio_get_di(1));

    // === Analog Test ===

    get_val = mio_get_ai(1);
    printf("AI1 = %d (%.3fV)\n\n", get_val, (float)get_val/1000);

    // Set AO1 to 5V

    set_val = 5000;
    printf("AO1 <-- %d\n", set_val);
    mio_set_ao(1, set_val);
    //usleep(50000);
    //printf("AO1 = %d\n", mio_get_ao(1));

    t0 = now_us();
    while (1) {
        get_val = mio_get_ai(1);
        elapsed = now_us() - t0;

        if ((get_val >= set_val - tolerance) && (get_val <= set_val + tolerance))
		{
            printf("AI1 = %d (after %lld us, tolerance %d)\n", get_val, elapsed, tolerance);
            break;
        }

        if (elapsed >= timeout) {
            printf("WARNING(TIMEOUT): AI1 did not become %d within %lld us, tolerance %d)\n", get_val, elapsed, tolerance);
            break;
        }
    }

    get_val = mio_get_ai(1);
    printf("AI1 = %d (%.3fV)\n\n", get_val, (float)get_val/1000);

    // Set AO1 to 10V

    set_val = 10000;
    printf("AO1 <-- %d\n", set_val);
    mio_set_ao(1, set_val);
    //usleep(50000);
    //printf("AO1 = %d\n", mio_get_ao(1));

    t0 = now_us();
    while (1) {
        get_val = mio_get_ai(1);
        elapsed = now_us() - t0;

        if ((get_val >= set_val - tolerance) && (get_val <= set_val + tolerance))
		{
            printf("AI1 = %d (after %lld us, tolerance %d)\n", get_val, elapsed, tolerance);
            break;
        }

        if (elapsed >= timeout) {
            printf("WARNING(TIMEOUT): AI1 did not become %d within %lld us, tolerance %d)\n", get_val, elapsed, tolerance);
            break;
        }
    }

    get_val = mio_get_ai(1);
    printf("AI1 = %d (%.3fV)\n\n", get_val, (float)get_val/1000);

    // Set AO1 to 0V

    tolerance = 20;		// ±200 counts = ±0.2% of 0–10000 range
    set_val = 0;
    printf("AO1 <-- %d\n", set_val);
    mio_set_ao(1, set_val);
    //usleep(50000);
    //printf("AO1 = %d\n", mio_get_ao(1));

    t0 = now_us();
    while (1) {
        get_val = mio_get_ai(1);
        elapsed = now_us() - t0;

        if ((get_val >= set_val - tolerance) && (get_val <= set_val + tolerance))
		{
            printf("AI1 = %d (after %lld us, tolerance %d)\n", get_val, elapsed, tolerance);
            break;
        }

        if (elapsed >= timeout) {
            printf("WARNING(TIMEOUT): AI1 did not become %d within %lld us, tolerance %d)\n", get_val, elapsed, tolerance);
            break;
        }
    }

    get_val = mio_get_ai(1);
    printf("AI1 = %d (%.3fV)\n\n", get_val, (float)get_val/1000);
	
    return 0;
}

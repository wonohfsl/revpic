#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <sched.h>

#include "revpi_connect4_mio_map.h"
#include "revpi_mio.h"   // your driver

/* 1 ms cycle time */
//static const struct timespec cycle = {0, 1 * 1000 * 1000};
/* 30 s cycle time */
static const struct timespec cycle = {30, 0};

int main(void)
{
    /* Initialize driver */
    if (mio_init() < 0) {
        fprintf(stderr, "Failed to init MIO driver\n");
        return 1;
    }

    /* Set real-time priority */
    struct sched_param sp = { .sched_priority = 80 };
    pthread_setschedparam(pthread_self(), SCHED_FIFO, &sp);

    printf("Starting real-time loop...\n");

    while (1) {

        /* 1. Read DI1 */
        int di1_before = mio_get_di(1);
        printf("DI1 before = %d\n", di1_before);

        /* 2. Write DO3 = 1 */
        mio_set_do(3, 1);
        printf("DO3 = 1\n");

        /* 3. Read DI1 */
        int di1_after1 = mio_get_di(1);
        printf("DI1 after DO3=1 = %d\n", di1_after1);

        /* 4. Write DO3 = 0 */
        mio_set_do(3, 0);
        printf("DO3 = 0\n");

        /* 5. Read DI1 */
        int di1_after2 = mio_get_di(1);
        printf("DI1 after DO3=0 = %d\n", di1_after2);

        /* deterministic sleep */
        clock_nanosleep(CLOCK_MONOTONIC, 0, &cycle, NULL);
    }

    mio_close();
    return 0;
}

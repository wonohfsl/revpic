// machine.c
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>   // usleep
#include <time.h>     // time()
#include "mio.h"
#include "ro.h"
#include "motion.h"
#include "tilt.h"

int main(void)
{
    printf("[Machine] Starting machine daemon\n");

    if (mio_init() < 0) {
        printf("[Machine] Error: mio_init() failed\n");
        return 1;
    }

    if (ro_init() < 0) {
        printf("[Machine] Error: ro_init() failed\n");
        return 1;
    }

    printf("[Machine] HAL initialized\n");

    // Start Tilt(70) → target ≈ 7.0 V
    Tilt_Start(70);

    time_t startTime = time(NULL);
    bool paused = false;

    while (!Tilt_IsDone()) {
        usleep(100000);  // 100 ms loop

        time_t now = time(NULL);
        double elapsed = difftime(now, startTime);

        // Auto-pause after 10 seconds
        if (!paused && elapsed >= 10.0) {
            printf("[Machine] Auto-pause after %.0f seconds\n", elapsed);
            paused = true;
        }

        // Call tilt controller with pause flag
        Tilt_Update(paused);
    }

    printf("[Machine] Tilt controller finished. Exiting.\n");
    return 0;
}

/**
 * @file main.c
 * @brief Main loop for the machine controller.
 *
 * Initializes the control state machine and continuously monitors
 * ESTOP and executes periodic ticks.
 */

#include <stdio.h>
#include <unistd.h>
#include "control.h"

static int ReadEStopButton(void); // TODO: connect to motion.c later

/**
 * @brief Program entry point.
 *
 * Initializes the control module and enters the main loop.
 * Periodically checks ESTOP and calls Control_Tick().
 *
 * @return Always returns 0.
 */
int main(void)
{
    Control_Init();

    while (1) {
        int estop_pressed = ReadEStopButton();
        if (estop_pressed) {
            Control_NotifyEStopActive();
        }

        Control_Tick();

        MachineStatus_t st = Control_GetStatus();
        printf("Machine status: %d\n", (int)st);

        usleep(100000); // 100 ms tick
    }

    return 0;
}

/**
 * @brief Read the ESTOP button state.
 *
 * Dummy implementation. Real implementation will call motion.c.
 *
 * @return 1 if ESTOP is pressed, 0 otherwise.
 */
static int ReadEStopButton(void)
{
    // TODO: call motion.c: ReadEStopButton()
    return 0;
}

#include <stdio.h>
#include <unistd.h>
#include "../src/include/control.h"

static void print_status(const char *label)
{
    MachineStatus_t st = Control_GetStatus();
    printf("[STATUS] %-12s -> %d\n", label, st);
}

int main(void)
{
    printf("=== TEST: MACHINE ARCHITECTURE A ===\n");

    Control_Init();
    print_status("Init");

    printf("\n-- Step 1: Check Calibration (expect fault because not calibrated) --\n");
    int rc = Control_CheckCalibration();
    print_status("CheckCalibration");
    printf("Return: %d\n", rc);

    printf("\n-- Step 2: Calibrate Tilt + Rotate --\n");
    Control_CalibrateTilt();
    Control_CalibrateRotate();
    rc = Control_CheckCalibration();
    print_status("CheckCalibration");
    printf("Return: %d\n", rc);

    printf("\n-- Step 3: Home Machine --\n");
    rc = Control_Home();
    print_status("Home");
    printf("Return: %d\n", rc);

    printf("\n-- Step 4: Start Session --\n");
    SessionConfig_t cfg = {
        .tilt_degree = 30,
        .rotate_dir = ROTATE_DIR_CW,
        .rotate_num = 5
    };

    rc = Control_StartSession(&cfg);
    print_status("StartSession");
    printf("Return: %d\n", rc);

    printf("\n-- Step 5: Tick until running completes --\n");
    for (int i = 0; i < 10; i++) {
        Control_Tick();
        print_status("Tick");
        usleep(100000);
    }

    printf("\n-- Step 6: Pause/Resume Test --\n");
    Control_Init();
    Control_CalibrateTilt();
    Control_CalibrateRotate();
    Control_Home();
    Control_StartSession(&cfg);

    Control_Tick();
    Control_PauseSession();
    print_status("Pause");

    Control_ResumeSession();
    print_status("Resume");

    printf("\n-- Step 7: Stop Test --\n");
    Control_StopSession();
    print_status("Stop");

    printf("\n-- Step 8: ESTOP Test --\n");
    Control_Init();
    Control_NotifyEStopActive();
    Control_Tick();
    print_status("ESTOP");

    printf("\n=== END TEST ===\n");
    return 0;
}

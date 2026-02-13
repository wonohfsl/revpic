# Machine Container Architecture

## 1. Architecture overview

- **Compute platform:** RevPi Connect 4 running BalenaOS  
- **Containers:**
  - **agent-api (rider):**  
    - Owns gRPC API to external world  
    - Sends high‑level commands to `machine` (start/pause/resume/stop, calibration, home, status)  
  - **machine:**  
    - Owns state machine and safety logic  
    - Talks to RevPi I/O via HAL (`motion.c`, `mio.c`, `ro.c`, `tilt.c`, `piControlIf.c`)  
    - Continuously monitors ESTOP  
    - Executes sessions (T‑axis tilt + R‑axis rotate)  

- **Axes:**
  - **R‑axis:** DC motor, 1 rpm, HOME via proximity sensor  
  - **T‑axis:** Linear actuator, 0–10 V position feedback, HOME via proximity sensor  

---

## 2. Architecture diagram (high level)

```text
+---------------------------+          +---------------------------+
|        agent-api          |          |          machine          |
|        (rider)            |          |        container          |
|                           |          |                           |
|  +---------------------+  |  gRPC    |  +---------------------+  |
|  |  Session / Control  |<----------->|  |  Command Handler    |  |
|  |  API (rider side)   |  |          |  |  (control.c)        |  |
|  +---------------------+  |          |  +----------+----------+  |
|                           |          |             |             |
+---------------------------+          |   State Machine &         |
                                       |   Safety Supervisor       |
                                       |   (control.c, main.c)     |
                                       |             |             |
                                       |   +---------+----------+  |
                                       |   |   Axis & Calib     |  |
                                       |   |   Logic            |  |
                                       |   |   (control_*.c,    |  |
                                       |   |    calibration_*.c)|  |
                                       |   +---------+----------+  |
                                       |             |             |
                                       |   +---------+----------+  |
                                       |   |   HAL / I/O        |  |
                                       |   |   motion.c, mio.c, |  |
                                       |   |   ro.c, tilt.c,    |  |
                                       |   |   piControlIf.c    |  |
                                       |   +--------------------+  |
                                       +---------------------------+
```

---

## 3. State machine (machine container)

### 3.1 States

- **READY:** Idle, safe, homed or homable, waiting for commands  
- **RUNNING:** Executing a session (tilt + rotate loop)  
- **PAUSED:** Motion paused, session context retained  
- **DONE:** Session completed successfully  
- **ESTOP:** Emergency stop active (latched until cleared/reset)  
- **FAULT:** Any error (home failure, calibration failure, invalid command, etc.)  

### 3.2 Events / commands

- **check_calibration** → `CheckCalibration()`  
- **check_home** → `CheckHome()`  
- **calibrate_tilt** → `CalibrateTilt()`  
- **calibrate_rotate** → `CalibrateRotate()`  
- **home** → `HomeTilt()` + `HomeRotate()`  
- **start(session)** → `StartSession()`  
- **pause** → `PauseSession()`  
- **resume** → `ResumeSession()`  
- **stop** → `StopSession()`  
- **ESTOP pressed** → `MonitorEStop()` → state = `ESTOP`  

### 3.3 State transitions (simplified)

```text
[READY] --start--> validate session
    - CheckSession()
    - CheckCalibration()
    - CheckHome()
    if any error -> [FAULT]
    else -> [RUNNING]

[RUNNING]
    - Execute Tilt(target_degree)
    - Loop rotate_num times: Rotate(rotate_dir)
    - On success -> [DONE]
    - On pause cmd -> PauseTilt(), PauseRotate() -> [PAUSED]
    - On stop cmd -> [FAULT]
    - On ESTOP -> [ESTOP]

[PAUSED]
    - resume cmd and previous state == RUNNING -> [RUNNING]
    - stop cmd -> [FAULT]
    - ESTOP -> [ESTOP]

[DONE]
    - home / start / check_* / calibrate_* allowed
    - ESTOP -> [ESTOP]

[FAULT]
    - Requires explicit recovery (e.g., home, recalibrate, or reset)
    - ESTOP -> [ESTOP]

[ESTOP]
    - Motion inhibited
    - Requires physical ESTOP reset + software reset path
```

---

## 4. Proposed file structure (refined)

```text
machine/
│
├── src/
│   ├── app/
│   │   ├── main.c              // process entry, main loop, ESTOP monitor tick
│   │   └── control.c           // state machine, command dispatch, session logic
│   │
│   ├── control/
│   │   ├── control_tilt.c      // T-axis high-level control
│   │   └── control_rotate.c    // R-axis high-level control
│   │
│   ├── calibration/
│   │   ├── calibration_tilt.c  // Tilt calibration, calibration.json handling (tilt)
│   │   └── calibration_rotate.c// Rotate calibration, calibration.json handling (rotate)
│   │
│   ├── hal/
│   │   ├── motion.c            // already implemented low-level helpers
│   │   ├── mio.c
│   │   ├── ro.c
│   │   ├── tilt.c
│   │   └── piControlIf.c
│   │
│   ├── include/
│   │   ├── app_main.h
│   │   ├── control.h
│   │   ├── control_tilt.h
│   │   ├── control_rotate.h
│   │   ├── calibration_tilt.h
│   │   ├── calibration_rotate.h
│   │   ├── mio.h
│   │   ├── ro.h
│   │   ├── motion.h
│   │   └── tilt.h
│   │
│   └── config/
│       ├── mio_addr.h
│       ├── ro_addr.h
│       └── calibration_paths.h // path(s) to calibration.json
│
├── data/
│   ├── machine/
│       └── calibration.json
│
└── test/
    ├── test_control.c
    ├── test_control_tilt.c
    ├── test_control_rotate.c
    ├── test_calibration_tilt.c
    ├── test_calibration_rotate.c
    ├── test_mio.c
    ├── test_ro.c
    ├── test_motion_a.c
    └── test_motion_hw.c
```

---

### 5. C skeletons (dummy, testable, no hardware/grpc yet)

#### 5.1 `src/include/control.h`

```c
#ifndef CONTROL_H
#define CONTROL_H

#include <stdint.h>

typedef enum {
    MACHINE_STATUS_READY = 0,
    MACHINE_STATUS_RUNNING,
    MACHINE_STATUS_PAUSED,
    MACHINE_STATUS_DONE,
    MACHINE_STATUS_ESTOP,
    MACHINE_STATUS_FAULT
} MachineStatus_t;

typedef enum {
    ROTATE_DIR_CW = 0,
    ROTATE_DIR_CCW
} RotateDirection_t;

typedef struct {
    int tilt_degree;
    RotateDirection_t rotate_dir;
    int rotate_num;
} SessionConfig_t;

void Control_Init(void);
void Control_Tick(void);  // called periodically from main loop

// Command entry points (to be called from gRPC handlers later)
int Control_CheckCalibration(void);
int Control_CheckHome(void);
int Control_CalibrateTilt(void);
int Control_CalibrateRotate(void);
int Control_Home(void);

int Control_StartSession(const SessionConfig_t *cfg);
int Control_PauseSession(void);
int Control_ResumeSession(void);
int Control_StopSession(void);

MachineStatus_t Control_GetStatus(void);

void Control_NotifyEStopActive(void); // called when ESTOP detected

#endif // CONTROL_H
```

#### 5.2 `src/app/main.c`

```c
#include <stdio.h>
#include <unistd.h>
#include "control.h"

static int ReadEStopButton(void); // TODO: connect to motion.c later

int main(void)
{
    Control_Init();

    while (1) {
        // TODO: replace with real ESTOP read from motion.c
        int estop_pressed = ReadEStopButton();
        if (estop_pressed) {
            Control_NotifyEStopActive();
        }

        Control_Tick();

        // Simple debug print
        MachineStatus_t st = Control_GetStatus();
        printf("Machine status: %d\n", (int)st);

        usleep(100000); // 100 ms tick
    }

    return 0;
}

static int ReadEStopButton(void)
{
    // TODO: call motion.c: ReadEStopButton()
    // For now, always not pressed
    return 0;
}
```

#### 5.3 `src/app/control.c`

```c
#include <stdio.h>
#include "control.h"
#include "control_tilt.h"
#include "control_rotate.h"
#include "calibration_tilt.h"
#include "calibration_rotate.h"

static MachineStatus_t g_status = MACHINE_STATUS_READY;
static SessionConfig_t g_session;
static int g_current_rotate_count = 0;
static int g_estop_latched = 0;

static int CheckSession(const SessionConfig_t *cfg);

void Control_Init(void)
{
    g_status = MACHINE_STATUS_READY;
    g_estop_latched = 0;
    g_current_rotate_count = 0;
}

void Control_Tick(void)
{
    if (g_estop_latched) {
        g_status = MACHINE_STATUS_ESTOP;
        return;
    }

    if (g_status == MACHINE_STATUS_RUNNING) {
        // Simple dummy progression: pretend rotation steps complete over ticks
        if (g_current_rotate_count < g_session.rotate_num) {
            g_current_rotate_count++;
        } else {
            g_status = MACHINE_STATUS_DONE;
        }
    }
}

int Control_CheckCalibration(void)
{
    int tilt_ok = CalibrationTilt_Check();
    int rot_ok  = CalibrationRotate_Check();

    if (!tilt_ok || !rot_ok) {
        g_status = MACHINE_STATUS_FAULT;
        return -1;
    }
    return 0;
}

int Control_CheckHome(void)
{
    int tilt_ok = ControlTilt_CheckHome();
    int rot_ok  = ControlRotate_CheckHome();

    if (!tilt_ok || !rot_ok) {
        g_status = MACHINE_STATUS_FAULT;
        return -1;
    }
    return 0;
}

int Control_CalibrateTilt(void)
{
    if (CalibrationTilt_Run() != 0) {
        g_status = MACHINE_STATUS_FAULT;
        return -1;
    }
    return 0;
}

int Control_CalibrateRotate(void)
{
    if (CalibrationRotate_Run() != 0) {
        g_status = MACHINE_STATUS_FAULT;
        return -1;
    }
    return 0;
}

int Control_Home(void)
{
    int rc = 0;
    rc |= ControlTilt_Home();
    rc |= ControlRotate_Home();

    if (rc != 0) {
        g_status = MACHINE_STATUS_FAULT;
        return -1;
    }
    g_status = MACHINE_STATUS_READY;
    return 0;
}

int Control_StartSession(const SessionConfig_t *cfg)
{
    if (g_status != MACHINE_STATUS_READY &&
        g_status != MACHINE_STATUS_DONE) {
        g_status = MACHINE_STATUS_FAULT;
        return -1;
    }

    if (CheckSession(cfg) != 0) {
        g_status = MACHINE_STATUS_FAULT;
        return -1;
    }

    if (Control_CheckCalibration() != 0) {
        return -1;
    }

    if (Control_CheckHome() != 0) {
        return -1;
    }

    g_session = *cfg;
    g_current_rotate_count = 0;

    // Dummy tilt call
    ControlTilt_TiltTo(cfg->tilt_degree);

    // In real implementation, rotation loop would be time-based
    g_status = MACHINE_STATUS_RUNNING;
    return 0;
}

int Control_PauseSession(void)
{
    if (g_status != MACHINE_STATUS_RUNNING) {
        g_status = MACHINE_STATUS_FAULT;
        return -1;
    }

    ControlTilt_Pause();
    ControlRotate_Pause();

    g_status = MACHINE_STATUS_PAUSED;
    return 0;
}

int Control_ResumeSession(void)
{
    if (g_status != MACHINE_STATUS_PAUSED) {
        g_status = MACHINE_STATUS_FAULT;
        return -1;
    }

    // In real implementation, resume motion from stored positions
    g_status = MACHINE_STATUS_RUNNING;
    return 0;
}

int Control_StopSession(void)
{
    // For now, just mark as fault
    g_status = MACHINE_STATUS_FAULT;
    return 0;
}

MachineStatus_t Control_GetStatus(void)
{
    return g_status;
}

void Control_NotifyEStopActive(void)
{
    g_estop_latched = 1;
}

static int CheckSession(const SessionConfig_t *cfg)
{
    if (!cfg) return -1;
    if (cfg->rotate_num < 0) return -1;
    if (cfg->tilt_degree < 0 || cfg->tilt_degree > 90) {
        // arbitrary limit for now
        return -1;
    }
    return 0;
}
```

---

#### 5.4 `src/include/control_tilt.h`

```c
#ifndef CONTROL_TILT_H
#define CONTROL_TILT_H

int ControlTilt_CheckHome(void);
int ControlTilt_Home(void);
int ControlTilt_TiltTo(int degree);
int ControlTilt_ReadPosition(int *degree_out);
int ControlTilt_Pause(void);

#endif // CONTROL_TILT_H
```

#### 5.5 `src/control/control_tilt.c`

```c
#include <stdio.h>
#include "control_tilt.h"

static int g_tilt_position_deg = 0;
static int g_tilt_is_homed = 0;

int ControlTilt_CheckHome(void)
{
    // TODO: use ReadHomeTilt() from motion.c
    // For now, assume homed if position == 0
    return (g_tilt_position_deg == 0);
}

int ControlTilt_Home(void)
{
    // TODO: move actuator until HOME sensor active
    g_tilt_position_deg = 0;
    g_tilt_is_homed = 1;
    printf("Tilt homed (dummy)\n");
    return 0;
}

int ControlTilt_TiltTo(int degree)
{
    if (!g_tilt_is_homed) {
        return -1;
    }
    // TODO: convert degree to voltage / position and command actuator
    g_tilt_position_deg = degree;
    printf("Tilt moved to %d deg (dummy)\n", degree);
    return 0;
}

int ControlTilt_ReadPosition(int *degree_out)
{
    if (!degree_out) return -1;
    *degree_out = g_tilt_position_deg;
    return 0;
}

int ControlTilt_Pause(void)
{
    // TODO: stop actuator motion, keep current position
    printf("Tilt paused at %d deg (dummy)\n", g_tilt_position_deg);
    return 0;
}
```

---

#### 5.6 `src/include/control_rotate.h`

```c
#ifndef CONTROL_ROTATE_H
#define CONTROL_ROTATE_H

#include "control.h"

int ControlRotate_CheckHome(void);
int ControlRotate_Home(void);
int ControlRotate_RotateStep(RotateDirection_t dir);
int ControlRotate_ReadPosition(int *count_out);
int ControlRotate_Pause(void);

#endif // CONTROL_ROTATE_H
```

#### 5.7 `src/control/control_rotate.c`

```c
#include <stdio.h>
#include "control_rotate.h"

static int g_rotate_count = 0;
static int g_rotate_is_homed = 0;

int ControlRotate_CheckHome(void)
{
    // TODO: use ReadHomeRotate() from motion.c
    // For now, assume homed if count == 0
    return (g_rotate_count == 0);
}

int ControlRotate_Home(void)
{
    // TODO: rotate CW until HOME sensor active
    g_rotate_count = 0;
    g_rotate_is_homed = 1;
    printf("Rotate homed (dummy)\n");
    return 0;
}

int ControlRotate_RotateStep(RotateDirection_t dir)
{
    if (!g_rotate_is_homed) {
        return -1;
    }

    if (dir == ROTATE_DIR_CW) {
        g_rotate_count++;
    } else {
        g_rotate_count--;
    }

    // TODO: command relay / motor for one step (time-based)
    printf("Rotate step %s, count=%d (dummy)\n",
           (dir == ROTATE_DIR_CW) ? "CW" : "CCW",
           g_rotate_count);
    return 0;
}

int ControlRotate_ReadPosition(int *count_out)
{
    if (!count_out) return -1;
    *count_out = g_rotate_count;
    return 0;
}

int ControlRotate_Pause(void)
{
    // TODO: stop motor
    printf("Rotate paused at count=%d (dummy)\n", g_rotate_count);
    return 0;
}
```

---

#### 5.8 `src/include/calibration_tilt.h`

```c
#ifndef CALIBRATION_TILT_H
#define CALIBRATION_TILT_H

int CalibrationTilt_Check(void);
int CalibrationTilt_Run(void);

#endif // CALIBRATION_TILT_H
```

#### 5.9 `src/calibration/calibration_tilt.c`

```c
#include <stdio.h>
#include "calibration_tilt.h"

// Dummy in-memory flag instead of real calibration.json
static int g_tilt_calibrated = 0;

int CalibrationTilt_Check(void)
{
    // TODO: read calibration.json and validate tilt entries
    return g_tilt_calibrated;
}

int CalibrationTilt_Run(void)
{
    // TODO: perform real calibration and write calibration.json
    g_tilt_calibrated = 1;
    printf("Tilt calibration done (dummy)\n");
    return 0;
}
```

---

#### 5.10 `src/include/calibration_rotate.h`

```c
#ifndef CALIBRATION_ROTATE_H
#define CALIBRATION_ROTATE_H

int CalibrationRotate_Check(void);
int CalibrationRotate_Run(void);

#endif // CALIBRATION_ROTATE_H
```

#### 5.11 `src/calibration/calibration_rotate.c`

```c
#include <stdio.h>
#include "calibration_rotate.h"

// Dummy in-memory flag instead of real calibration.json
static int g_rotate_calibrated = 0;

int CalibrationRotate_Check(void)
{
    // TODO: read calibration.json and validate rotate entries
    return g_rotate_calibrated;
}

int CalibrationRotate_Run(void)
{
    // TODO: perform real calibration and write calibration.json
    g_rotate_calibrated = 1;
    printf("Rotate calibration done (dummy)\n");
    return 0;
}
```

---

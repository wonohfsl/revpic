# Control Module Documentation

## Overview

The control module consists of three main components that work together to orchestrate the machine's operation:

1. **control.c/h** - High-level state machine and session orchestrator
2. **control_tilt.c/h** - Tilt axis (T-axis) control for 0-75° positioning
3. **control_rotate.c/h** - Rotation axis (R-axis) control for 360° rotation

The system uses a **non-blocking architecture** that allows responsive pause/resume functionality without blocking the main thread or gRPC handlers. This is essential for real-time control of hardware via RevPi Connect 4, MIO, and RO modules.

---

## Architecture

### State Machine Flow

```
IDLE → HOME_TILT → HOME_ROTATE → READY → TILT → ROTATE → DONE
                                    ↓        ↓        ↓
                                  PAUSED ← PAUSED ← PAUSED
```

### Result Types

Both tilt and rotate modules use similar result enumerations:

**TiltResult_t:**
- `TILT_OK` - Motion completed successfully
- `TILT_RUNNING` - Motion is in progress (call Service again)
- `TILT_PAUSED` - Motion paused due to pause request
- `TILT_STOPPED` - Motion stopped due to stop request
- `TILT_ERROR` - Motion failed due to error or invalid input

**RotateResult_t:**
- `ROTATE_OK` - Motion completed successfully
- `ROTATE_RUNNING` - Motion is in progress (call Service again)
- `ROTATE_PAUSED` - Motion paused due to pause request
- `ROTATE_STOPPED` - Motion stopped due to stop request
- `ROTATE_ERROR` - Motion failed due to error or invalid input

**MachineStatus_t:**
- `MACHINE_STATUS_READY` - Idle and ready for a new session
- `MACHINE_STATUS_RUNNING` - Session is currently executing
- `MACHINE_STATUS_PAUSED` - Session is paused
- `MACHINE_STATUS_DONE` - Session completed normally
- `MACHINE_STATUS_ESTOP` - Emergency stop is active
- `MACHINE_STATUS_FAULT` - Fault condition detected

---

## 1. Control Module (control.c/h)

The main control module orchestrates the entire machine state machine, managing sessions, homing, and coordination between axes.

### Core Functions

#### Initialization

```c
void Control_Init(void);
```

Initializes the control state machine. Must be called before any other control functions.

**Example:**
```c
Control_Init();
```

---

#### Non-Blocking Orchestrator

```c
void Control_Tick(void);
```

The heart of the non-blocking architecture. Call this periodically (e.g., every 10-100ms) to:
- Monitor ESTOP conditions
- Advance homing sequences
- Service tilt motion
- Service rotate motion
- Handle phase transitions

**Example:**
```c
while (1) {
    Control_Tick();
    usleep(10000); // 10ms cycle
}
```

---

### Session Management

#### Start Session

```c
int Control_StartSession(const SessionConfig_t *cfg);
```

Starts a new machine session with the specified configuration.

**Session Configuration:**
```c
typedef struct {
    int tilt_degree;              // Target tilt angle (0-75°)
    RotateDirection_t rotate_dir; // CW or CCW
    int rotate_num;               // Number of rotation steps
} SessionConfig_t;
```

**Behavior:**
1. Validates that machine is in READY or DONE state
2. Checks calibration and home status
3. Begins tilt motion to target degree
4. After tilt completes, begins rotation
5. Returns immediately (non-blocking)

**Returns:**
- `0` on success (session started or completed immediately)
- `-1` on failure (invalid state, bad config, or not homed)

**Example:**
```c
SessionConfig_t session = {
    .tilt_degree = 45,
    .rotate_dir = ROTATE_DIR_CW,
    .rotate_num = 3
};

if (Control_StartSession(&session) == 0) {
    printf("Session started\n");
    
    // Service the session with Control_Tick()
    while (Control_GetStatus() == MACHINE_STATUS_RUNNING) {
        Control_Tick();
        usleep(10000);
    }
    
    if (Control_GetStatus() == MACHINE_STATUS_DONE) {
        printf("Session completed successfully\n");
    }
}
```

---

#### Pause Session

```c
int Control_PauseSession(void);
```

Pauses the currently running session. The machine stops at its current position and can be resumed later.

**Behavior:**
- Stops both tilt and rotate motion immediately
- Preserves session state for resume
- Changes status to `MACHINE_STATUS_PAUSED`

**Returns:**
- `0` on success
- `-1` if not currently running

**Example:**
```c
if (Control_GetStatus() == MACHINE_STATUS_RUNNING) {
    if (Control_PauseSession() == 0) {
        printf("Session paused\n");
    }
}
```

---

#### Resume Session

```c
int Control_ResumeSession(void);
```

Resumes a paused session from where it stopped.

**Behavior:**
- Validates machine is in PAUSED state
- Restarts motion for current phase (tilt or rotate)
- Uses saved session configuration
- Changes status to `MACHINE_STATUS_RUNNING`

**Returns:**
- `0` on success
- `-1` if not paused or resume failed

**Example:**
```c
if (Control_GetStatus() == MACHINE_STATUS_PAUSED) {
    if (Control_ResumeSession() == 0) {
        printf("Session resumed\n");
        
        // Continue servicing
        while (Control_GetStatus() == MACHINE_STATUS_RUNNING) {
            Control_Tick();
            usleep(10000);
        }
    }
}
```

---

#### Stop Session

```c
int Control_StopSession(void);
```

Stops the current session immediately and aborts all motion.

**Behavior:**
- Stops both axes immediately
- Clears session state
- Changes status to `MACHINE_STATUS_FAULT`
- Session cannot be resumed (unlike pause)

**Returns:**
- Always returns `0`

**Example:**
```c
Control_StopSession();
printf("Session stopped and aborted\n");
```

---

### Session Management Example

```c
// Start a session
SessionConfig_t session = {
    .tilt_degree = 60,
    .rotate_dir = ROTATE_DIR_CW,
    .rotate_num = 5
};

Control_StartSession(&session);

// Run for a while
for (int i = 0; i < 100 && Control_GetStatus() == MACHINE_STATUS_RUNNING; i++) {
    Control_Tick();
    usleep(10000);
}

// Pause after 1 second
Control_PauseSession();
printf("Paused at tilt=%.2f°\n", ControlTilt_ReadDegree());

// Wait 2 seconds
sleep(2);

// Resume
Control_ResumeSession();

// Continue until done
while (Control_GetStatus() == MACHINE_STATUS_RUNNING) {
    Control_Tick();
    usleep(10000);
}

printf("Session completed\n");
```

---

### Homing Functions

#### Non-Blocking Home

```c
int Control_BeginHome(void);
```

Begins a non-blocking homing sequence for both axes.

**Behavior:**
1. Validates machine is in READY or DONE state
2. Starts tilt homing (pulls IN until HOME sensor triggers)
3. After tilt homes, starts rotate homing
4. Returns immediately

**Returns:**
- `0` on success
- `-1` on failure

**Example:**
```c
if (Control_BeginHome() == 0) {
    printf("Homing started\n");
    
    while (Control_GetStatus() == MACHINE_STATUS_RUNNING) {
        Control_Tick();
        usleep(10000);
    }
    
    if (Control_GetStatus() == MACHINE_STATUS_READY) {
        printf("Homing complete\n");
    }
}
```

---

#### Blocking Home

```c
int Control_Home(void);
```

Blocking wrapper that homes both axes and waits for completion.

**Returns:**
- `0` on success
- `-1` on failure

**Example:**
```c
if (Control_Home() == 0) {
    printf("Machine homed successfully\n");
}
```

---

### Status and Utility Functions

```c
MachineStatus_t Control_GetStatus(void);
int Control_CheckCalibration(void);
int Control_CheckHome(void);
int Control_CalibrateTilt(void);
int Control_CalibrateRotate(void);
void Control_NotifyEStopActive(void);
```

---

## 2. Control Tilt Module (control_tilt.c/h)

Controls the tilt axis (T-axis), which tilts a chair from 0 to 75 degrees using a linear actuator with position feedback.

### Key Features

- Non-blocking motion engine
- Stop-band compensation (compensates for actuator inertia)
- Pause/resume support
- Timeout and stall detection
- Voltage-to-degree calibration

---

### Homing Functions

#### Begin Homing

```c
TiltResult_t ControlTilt_BeginHome(void);
```

Starts a non-blocking homing sequence.

**Behavior:**
- Checks if already at HOME sensor → returns `TILT_OK` immediately
- Otherwise, activates relay to pull IN
- Returns `TILT_RUNNING` to indicate motion started

**Returns:**
- `TILT_OK` - Already homed
- `TILT_RUNNING` - Homing motion started
- `TILT_ERROR` - Failed to start

**Example:**
```c
TiltResult_t result = ControlTilt_BeginHome();
if (result == TILT_RUNNING) {
    printf("Tilt homing started\n");
}
```

---

#### Service Homing

```c
TiltResult_t ControlTilt_ServiceHome(void);
```

Services the homing sequence. Call repeatedly until it returns non-RUNNING.

**Behavior:**
- Checks HOME sensor each tick
- Monitors for pause/stop requests
- Detects stalls (no movement)
- Enforces timeout

**Returns:**
- `TILT_RUNNING` - Keep calling
- `TILT_OK` - Homing complete
- `TILT_PAUSED` - Paused by request
- `TILT_STOPPED` - Stopped by request
- `TILT_ERROR` - Timeout, stall, or sensor error

**Example:**
```c
ControlTilt_BeginHome();

TiltResult_t result;
while ((result = ControlTilt_ServiceHome()) == TILT_RUNNING) {
    usleep(100000); // 100ms
}

if (result == TILT_OK) {
    printf("Tilt axis homed\n");
}
```

---

#### Blocking Home

```c
int ControlTilt_Home(void);
```

Blocking wrapper that homes the tilt axis.

**Returns:**
- `0` on success
- Non-zero on failure

---

### Motion Functions

#### Begin Move to Voltage

```c
TiltResult_t ControlTilt_BeginMoveToVolt(float target_volt);
```

Starts a non-blocking move to a target voltage.

**Parameters:**
- `target_volt` - Target sensor voltage (typically 0.29V - 8.55V)

**Returns:**
- `TILT_OK` - Already at target
- `TILT_RUNNING` - Motion started
- `TILT_ERROR` - Not homed or invalid voltage

---

#### Begin Move to Degree

```c
TiltResult_t ControlTilt_BeginMoveToDegree(float target_degree);
```

Starts a non-blocking move to a target angle.

**Parameters:**
- `target_degree` - Target tilt angle in degrees (0-75°)

**Returns:**
- `TILT_OK` - Already at target
- `TILT_RUNNING` - Motion started
- `TILT_ERROR` - Not homed or invalid degree

**Example:**
```c
TiltResult_t result = ControlTilt_BeginMoveToDegree(45.0f);
if (result == TILT_RUNNING) {
    printf("Moving to 45 degrees\n");
}
```

---

#### Service Motion

```c
TiltResult_t ControlTilt_Service(float *actual_volt_out);
```

Services the non-blocking motion. Call repeatedly until it returns non-RUNNING.

**Parameters:**
- `actual_volt_out` - Optional output for current voltage (can be NULL)

**Returns:**
- `TILT_RUNNING` - Keep calling
- `TILT_OK` - Target reached
- `TILT_PAUSED` - Paused by request
- `TILT_STOPPED` - Stopped by request
- `TILT_ERROR` - Timeout, stall, or sensor error

**Example:**
```c
ControlTilt_BeginMoveToDegree(60.0f);

float actual_volt;
TiltResult_t result;
while ((result = ControlTilt_Service(&actual_volt)) == TILT_RUNNING) {
    printf("Moving... current=%.2fV\r", actual_volt);
    fflush(stdout);
    usleep(100000); // 100ms
}

if (result == TILT_OK) {
    printf("\nReached target: %.2f degrees\n", ControlTilt_ReadDegree());
}
```

---

#### Blocking Move Functions

```c
TiltResult_t ControlTilt_MoveToVolt(float target_volt, float *actual_volt_out);
TiltResult_t ControlTilt_MoveToDegree(float target_degree, float *actual_degree_out);
```

Blocking wrappers that move to target and wait for completion.

**Example:**
```c
float actual_degree;
TiltResult_t result = ControlTilt_MoveToDegree(30.0f, &actual_degree);

if (result == TILT_OK) {
    printf("Moved to %.2f degrees\n", actual_degree);
}
```

---

### Utility Functions

#### Read Position

```c
float ControlTilt_ReadDegree(void);
float ControlTilt_ReadVolt(void);
int ControlTilt_ReadPosition(float *degree_out);
```

Reads the current tilt position.

**Example:**
```c
float current_degree = ControlTilt_ReadDegree();
printf("Current tilt: %.2f degrees\n", current_degree);
```

---

#### Pause

```c
int ControlTilt_Pause(void);
```

Immediately stops tilt motion and preserves state.

**Example:**
```c
ControlTilt_Pause();
printf("Tilt paused at %.2f degrees\n", ControlTilt_ReadDegree());
```

---

#### Conversion Functions

```c
float ControlTilt_TiltToVolt(float degree);
float ControlTilt_VoltToTilt(float volts);
```

Converts between degrees and sensor voltage using calibration data.

---

### Complete Tilt Example

```c
// Initialize and home
ControlTilt_ApplyCalibration(&tilt_cal);
if (ControlTilt_Home() != 0) {
    printf("Homing failed\n");
    return -1;
}

// Move to 45 degrees (non-blocking)
ControlTilt_BeginMoveToDegree(45.0f);

float actual_volt;
TiltResult_t result;
int count = 0;

while ((result = ControlTilt_Service(&actual_volt)) == TILT_RUNNING) {
    count++;
    
    // Pause after 2 seconds
    if (count == 20) {
        ControlTilt_Pause();
        printf("\nPaused at %.2f degrees\n", ControlTilt_ReadDegree());
        sleep(2);
        ControlTilt_BeginMoveToDegree(45.0f); // Resume
    }
    
    usleep(100000); // 100ms
}

if (result == TILT_OK) {
    printf("Successfully reached 45 degrees\n");
}
```

---

## 3. Control Rotate Module (control_rotate.c/h)

Controls the rotation axis (R-axis), which rotates the chair 360 degrees using a stepper motor or DC motor with home sensor.

### Key Features

- Non-blocking motion engine
- Time-based rotation by degrees
- Full rotation until HOME sensor detection
- Pause/resume support
- Configurable RPM

---

### Homing Functions

#### Begin Homing

```c
RotateResult_t ControlRotate_BeginHome(void);
```

Starts a non-blocking homing sequence.

**Behavior:**
- Checks if already at HOME sensor → returns `ROTATE_OK` immediately
- Otherwise, rotates CW until HOME sensor triggers
- Returns `ROTATE_RUNNING` to indicate motion started

**Returns:**
- `ROTATE_OK` - Already homed
- `ROTATE_RUNNING` - Homing motion started
- `ROTATE_ERROR` - Failed to start

---

#### Service Homing

```c
RotateResult_t ControlRotate_ServiceHome(void);
```

Services the homing sequence. Call repeatedly until it returns non-RUNNING.

**Returns:**
- `ROTATE_RUNNING` - Keep calling
- `ROTATE_OK` - Homing complete
- `ROTATE_PAUSED` - Paused by request
- `ROTATE_STOPPED` - Stopped by request
- `ROTATE_ERROR` - Timeout or sensor error

**Example:**
```c
ControlRotate_BeginHome();

RotateResult_t result;
while ((result = ControlRotate_ServiceHome()) == ROTATE_RUNNING) {
    usleep(100000); // 100ms
}

if (result == ROTATE_OK) {
    printf("Rotation axis homed\n");
}
```

---

#### Blocking Home

```c
int ControlRotate_Home(void);
```

Blocking wrapper that homes the rotation axis.

---

### Motion Functions

#### Begin Rotate by Degrees

```c
RotateResult_t ControlRotate_BeginRotate(RotateDirection_t dir, float degrees);
```

Starts a non-blocking rotation by degrees using time-based estimation.

**Parameters:**
- `dir` - `ROTATE_DIR_CW` or `ROTATE_DIR_CCW`
- `degrees` - Rotation angle in degrees (positive value)

**Returns:**
- `ROTATE_OK` - degrees == 0 (nothing to do)
- `ROTATE_RUNNING` - Motion started
- `ROTATE_ERROR` - Not homed or invalid input

**Example:**
```c
// Rotate 180 degrees clockwise
RotateResult_t result = ControlRotate_BeginRotate(ROTATE_DIR_CW, 180.0f);
if (result == ROTATE_RUNNING) {
    printf("Rotating 180 degrees CW\n");
}
```

---

#### Begin Rotate One Full Revolution

```c
RotateResult_t ControlRotate_BeginRotateOne(RotateDirection_t dir);
```

Starts a non-blocking full rotation until the HOME sensor is detected.

**Behavior:**
1. If currently at HOME, rotates away until sensor clears
2. Continues rotating until HOME sensor triggers again
3. Completes exactly one full revolution

**Parameters:**
- `dir` - `ROTATE_DIR_CW` or `ROTATE_DIR_CCW`

**Returns:**
- `ROTATE_RUNNING` - Motion started
- `ROTATE_ERROR` - Not homed or invalid state

**Example:**
```c
// Rotate one full revolution clockwise
RotateResult_t result = ControlRotate_BeginRotateOne(ROTATE_DIR_CW);
if (result == ROTATE_RUNNING) {
    printf("Rotating one full revolution\n");
}
```

---

#### Service Motion

```c
RotateResult_t ControlRotate_Service(void);
```

Services the non-blocking rotation motion. Call repeatedly until it returns non-RUNNING.

**Returns:**
- `ROTATE_RUNNING` - Keep calling
- `ROTATE_OK` - Target reached or rotation complete
- `ROTATE_PAUSED` - Paused by request
- `ROTATE_STOPPED` - Stopped by request
- `ROTATE_ERROR` - Timeout or error

**Example:**
```c
ControlRotate_BeginRotate(ROTATE_DIR_CW, 270.0f);

RotateResult_t result;
while ((result = ControlRotate_Service()) == ROTATE_RUNNING) {
    printf("Rotating...\r");
    fflush(stdout);
    usleep(100000); // 100ms
}

if (result == ROTATE_OK) {
    printf("\nRotation complete\n");
}
```

---

### Utility Functions

#### Read Position

```c
int ControlRotate_ReadPosition(int *count_out);
```

Reads the current rotation count (implementation-dependent).

**Parameters:**
- `count_out` - Output pointer for rotation count

**Returns:**
- `0` on success
- `-1` on invalid pointer

---

#### Pause

```c
int ControlRotate_Pause(void);
```

Immediately stops rotation motion and preserves state.

**Example:**
```c
ControlRotate_Pause();
printf("Rotation paused\n");
```

---

#### Stop

```c
int ControlRotate_Stop(void);
```

Stops rotation motion and resets motion state.

**Example:**
```c
ControlRotate_Stop();
printf("Rotation stopped\n");
```

---

### Complete Rotate Example

```c
// Initialize and home
ControlRotate_ApplyCalibration(&rotate_cal);
if (ControlRotate_Home() != 0) {
    printf("Homing failed\n");
    return -1;
}

// Rotate 360 degrees (non-blocking time-based)
ControlRotate_BeginRotate(ROTATE_DIR_CW, 360.0f);

RotateResult_t result;
int ticks = 0;

while ((result = ControlRotate_Service()) == ROTATE_RUNNING) {
    ticks++;
    
    // Pause after 5 seconds
    if (ticks == 50) {
        ControlRotate_Pause();
        printf("\nRotation paused\n");
        sleep(2);
        
        // Resume with remaining rotation
        ControlRotate_BeginRotate(ROTATE_DIR_CW, 180.0f);
    }
    
    usleep(100000); // 100ms
}

if (result == ROTATE_OK) {
    printf("Rotation complete\n");
}

// Now rotate one full revolution until HOME
printf("\nRotating one full revolution...\n");
ControlRotate_BeginRotateOne(ROTATE_DIR_CCW);

while ((result = ControlRotate_Service()) == ROTATE_RUNNING) {
    printf(".\r");
    fflush(stdout);
    usleep(100000);
}

printf("\nFull revolution complete\n");
```

---

## Complete System Example

```c
#include "control.h"
#include "control_tilt.h"
#include "control_rotate.h"

int main(void)
{
    // 1. Initialize
    Control_Init();
    
    // 2. Load and apply calibration
    TiltCalibration_t tilt_cal;
    RotateCalibration_t rotate_cal;
    // ... load calibration from file ...
    ControlTilt_ApplyCalibration(&tilt_cal);
    ControlRotate_ApplyCalibration(&rotate_cal);
    
    // 3. Home the machine (blocking)
    printf("Homing machine...\n");
    if (Control_Home() != 0) {
        printf("Homing failed\n");
        return -1;
    }
    printf("Machine homed and ready\n");
    
    // 4. Start a session
    SessionConfig_t session = {
        .tilt_degree = 45,
        .rotate_dir = ROTATE_DIR_CW,
        .rotate_num = 3
    };
    
    printf("Starting session: tilt=45°, rotate=3x CW\n");
    if (Control_StartSession(&session) != 0) {
        printf("Failed to start session\n");
        return -1;
    }
    
    // 5. Run the session with pause capability
    int ticks = 0;
    int paused = 0;
    
    while (Control_GetStatus() == MACHINE_STATUS_RUNNING ||
           Control_GetStatus() == MACHINE_STATUS_PAUSED)
    {
        Control_Tick();
        
        ticks++;
        
        // Pause after 5 seconds for demonstration
        if (ticks == 500 && !paused) {
            Control_PauseSession();
            printf("\n=== Session paused ===\n");
            printf("Tilt: %.2f degrees\n", ControlTilt_ReadDegree());
            sleep(3);
            
            Control_ResumeSession();
            printf("=== Session resumed ===\n");
            paused = 1;
        }
        
        // Status update every second
        if (ticks % 100 == 0) {
            printf("Status: %s, Tilt: %.2f°\n",
                   Control_GetStatus() == MACHINE_STATUS_RUNNING ? "RUNNING" : "PAUSED",
                   ControlTilt_ReadDegree());
        }
        
        usleep(10000); // 10ms tick
    }
    
    // 6. Check result
    if (Control_GetStatus() == MACHINE_STATUS_DONE) {
        printf("\n=== Session completed successfully ===\n");
    } else {
        printf("\n=== Session failed ===\n");
        return -1;
    }
    
    return 0;
}
```

---

## Best Practices

### 1. Always Use Non-Blocking API in Production

```c
// ✅ GOOD: Non-blocking with Control_Tick()
Control_StartSession(&session);
while (Control_GetStatus() == MACHINE_STATUS_RUNNING) {
    Control_Tick();
    // Can handle gRPC requests here
    usleep(10000);
}

// ❌ BAD: Blocking API ties up the thread
Control_Home(); // Blocks for several seconds
```

### 2. Check Status Before State Changes

```c
// ✅ GOOD
if (Control_GetStatus() == MACHINE_STATUS_RUNNING) {
    Control_PauseSession();
}

// ❌ BAD: Pause when not running causes fault
Control_PauseSession();
```

### 3. Always Home Before Sessions

```c
// ✅ GOOD
Control_Home();
Control_StartSession(&session);

// ❌ BAD: Session will fail if not homed
Control_StartSession(&session);
```

### 4. Use Service Functions Consistently

```c
// ✅ GOOD: Regular service calls
while ((result = ControlTilt_Service(&volt)) == TILT_RUNNING) {
    usleep(100000); // Consistent 100ms
}

// ❌ BAD: Irregular service timing
ControlTilt_Service(&volt);
sleep(5); // Too long between service calls
ControlTilt_Service(&volt);
```

### 5. Handle All Result Codes

```c
// ✅ GOOD: Complete error handling
TiltResult_t result = ControlTilt_Service(&volt);
switch (result) {
    case TILT_OK:      printf("Complete\n"); break;
    case TILT_RUNNING: break; // Continue
    case TILT_PAUSED:  printf("Paused\n"); break;
    case TILT_STOPPED: printf("Stopped\n"); break;
    case TILT_ERROR:   printf("ERROR\n"); handle_error(); break;
}

// ❌ BAD: Incomplete handling
if (result == TILT_OK) {
    printf("Done\n");
} // What about errors?
```

---

## Troubleshooting

### Session Won't Start

**Check:**
1. Machine status is READY or DONE
2. Both axes are homed (`Control_CheckHome()`)
3. Calibration is loaded (`Control_CheckCalibration()`)
4. Session config is valid (tilt 0-75°, rotate_num ≥ 0)

### Motion Stalls or Times Out

**Check:**
1. Hardware connections (power, signals)
2. Relay activation (use multimeter or scope)
3. Sensor readings (ADC values, HOME sensor)
4. Calibration parameters (min/max volts, RPM)
5. Timeout margins in calibration

### Pause/Resume Not Working

**Check:**
1. Using non-blocking API (Begin + Service)
2. Calling `Control_Tick()` regularly
3. Checking status before pause/resume
4. Not mixing blocking and non-blocking calls

---

## Summary

The control module provides a robust, non-blocking architecture for managing the machine's dual-axis system:

- **control.c** orchestrates sessions and coordinates both axes
- **control_tilt.c** provides precise degree-based positioning with stop-band compensation
- **control_rotate.c** provides time-based and sensor-based rotation control

All modules support pause/resume functionality, making them suitable for real-time gRPC communication while controlling hardware reliably.

**Key Principles:**
1. Non-blocking Begin/Service pattern
2. Consistent result codes (OK, RUNNING, PAUSED, STOPPED, ERROR)
3. Regular `Control_Tick()` calls for orchestration
4. Calibration and homing before operations
5. Responsive pause/resume without blocking


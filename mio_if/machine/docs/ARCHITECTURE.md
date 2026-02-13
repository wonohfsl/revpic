# Machine Container Architecture

## 1. Architecture overview

- **Compute platform:** RevPi Connect 4 running BalenaOS  
- **Containers:**
  - **agent-api (rider):**  
    - Owns gRPC API to external world  
    - Sends high‑level commands to `machine` (start/pause/resume/stop, calibration, home, status)  
  - **machine:**  
    - Owns state machine and safety logic  
    - Implements non‑blocking tilt + rotate control  
    - Talks to RevPi I/O via HAL (`motion.c`, `mio.c`, `ro.c`, `tilt.c`, `piControlIf.c`)  
    - Continuously monitors ESTOP  
    - Executes sessions (T‑axis tilt + R‑axis rotate)  

- **Axes:**
  - **R‑axis:** DC motor, HOME via proximity sensor  
    - Non‑blocking engine implemented in `control_rotate.c`  
    - Degree-based motion via `ControlRotate_RotateMoveToDegree()` + `ControlRotate_Service()`  
  - **T‑axis:** Linear actuator, 0–10 V feedback, HOME via proximity sensor  
    - Non‑blocking engine implemented in `control_tilt.c`  
    - Voltage‑based motion via `BeginMoveToVolt()` + `Service()`  
    - Non‑blocking homing via `BeginHome()` + `ServiceHome()`  

- **Parts List:**

| Brand | Model / Part # | Description | Link |
|-------|----------------|-------------|------|
| **KUNBUS** | RevPi Connect 4 / KU-PR100380 | Compute module (CM4‑based), 32GB Flash, 8GB RAM, Wi‑Fi | `https://revolutionpi.com/en/revpi-connect-4/` [(revolutionpi.com in Bing)](https://www.bing.com/search?q="https%3A%2F%2Frevolutionpi.com%2Fen%2Frevpi-connect-4%2F") |
| **KUNBUS** | RevPi MIO / KU-PR100323 | Mixed I/O module (8 AI, 8 AO, 4 DIO) | [https://revolutionpi.com/en/revpi-mio/](https://revolutionpi.com/en/revpi-mio/) |
| **KUNBUS** | RevPi RO / KU-PR100386 | Relay Output module (4 relays) | [https://revolutionpi.com/en/revpi-ro/](https://revolutionpi.com/en/revpi-ro/) |
| **Bodine Electric** | 1255 / 33A5BEPM‑WX3 | R‑AXIS DC Motor (90V, 1.6A) | [https://www.bodine-electric.com](https://www.bodine-electric.com) |
| **ServoCity** | 1241091700 | T‑AXIS Linear Actuator (12V, 1570 lb, 0.3"/sec, 8" stroke) | [https://www.servocity.com/1241091700](https://www.servocity.com/1241091700) |
| **Weidmuller** | 1123610000 | DPDT Opto‑Isolated Relay (24V coil, 250V 8A contacts) | [https://www.farnell.com/datasheets/2635126.pdf](https://www.farnell.com/datasheets/2635126.pdf) |
| **OMRON** | E2B‑M12KS04‑M1‑B1 + XS2F‑M12PVC4S2M | PX2 R‑AXIS INDEX sensor (PNP/NO, 4mm) | [https://shop.a-aelectric.com/products/E2B-M12KS04-M1-B1](https://shop.a-aelectric.com/products/E2B-M12KS04-M1-B1) |
| **OMRON** | E2B‑M12KN08‑M1‑B1 + XS2F‑M12PVC4A2M | PX3 T‑AXIS HOME sensor (PNP/NO, 8mm) | [https://shop.a-aelectric.com/products/E2B-M12KN08-M1-B1](https://shop.a-aelectric.com/products/E2B-M12KN08-M1-B1) |

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
                                       |   (control.c)             |
                                       |             |             |
                                       |   +---------+----------+  |
                                       |   |  Axis Control      |  |
                                       |   |  (non-blocking)    |  |
                                       |   |  control_tilt.c    |  |
                                       |   |  control_rotate.c  |  |
                                       |   +---------+----------+  |
                                       |             |             |
                                       |   +---------+----------+  |
                                       |   |   HAL / I/O        |  |
                                       |   |   motion.c         |  |
                                       |   |   mio.c / ro.c     |  |
                                       |   |   tilt.c           |  |
                                       |   |   piControlIf.c    |  |
                                       |   +--------------------+  |
                                       +---------------------------+
```

 ---

 ## 3. Tilt non-blocking flow (overview)

 ### 3.1 State machine (tilt)

 ```text
 +------------------+        BeginHome        +------------------+
 |       IDLE       |------------------------>|      HOMING      |
 +------------------+                         +------------------+
     |  BeginMove                                         |
     v                                                    | home sensor
 +------------------+         Pause request               v
 |      MOVING      |-------------------------------+  +------------------+
 +------------------+                               |  |       IDLE       |
     |  stop/fault                                  |  +------------------+
     v                                              |
 +------------------+                               |
 |      ERROR       |<------------------------------+
 +------------------+
     ^
     |  Resume request
 +------------------+
 |      PAUSED      |
 +------------------+
 ```

 ### 3.2 Service tick flow (tilt)

 ```text
 ControlTilt_ServiceTick()
   - If state == IDLE: report OK
   - If pause requested: stop relay, state = PAUSED
   - If stop requested: stop relay, state = STOPPED
   - Read ADC -> volts
     - If invalid/out of range: stop relay, state = ERROR
   - If state == HOMING:
       - If home sensor active: stop relay, state = IDLE
       - Else if timeout or no-movement: state = ERROR
       - Else keep moving IN
   - If state == MOVING:
       - If target reached: stop relay, state = IDLE
       - Else if timeout or no-movement: state = ERROR
       - Else keep moving in direction
 ```

 ### 3.3 Timing model (tilt)

 - `Control_Tick()` runs at a fixed interval (for example 10 ms).
 - Tilt service uses the tick cadence; no sleeps inside service.
 - Timeout thresholds are derived from `sec_per_degree` and total delta with a minimum margin.

---

## 4. State machine (machine container)

### 4.1 States

- **READY:** Idle, safe, homed or homable  
- **RUNNING:** Non‑blocking session in progress  
- **PAUSED:** Motion paused (tilt or rotate)  
- **DONE:** Session completed  
- **ESTOP:** Emergency stop latched  
- **FAULT:** Any error (home failure, calibration failure, invalid command, etc.)  

### 4.2 Events / commands

- **check_calibration** → `CalibrationTilt_Check()`, `CalibrationRotate_Check()`  
- **check_home** → `ControlTilt_CheckHome()`, `ControlRotate_CheckHome()`  
- **calibrate_tilt** → `CalibrationTilt_Run()`  
- **calibrate_rotate** → `CalibrationRotate_Run()`  
- **home** → `Control_BeginHome()` (non‑blocking)  
- **start(session)** → `Control_StartSession()`  
- **pause** → `Control_PauseSession()`  
- **resume** → `Control_ResumeSession()`  
- **stop** → `Control_StopSession()`  
- **ESTOP pressed** → `Control_NotifyEStopActive()`  

### 4.3 State transitions (updated to match real code)

```text
[READY] --start--> validate session
    - CheckSession()
    - CheckCalibration()
    - CheckHome()
    if any error -> [FAULT]
    else -> [RUNNING] → CONTROL_PHASE_TILT

[RUNNING]
    CONTROL_PHASE_TILT:
        - ControlTilt_Service()
        - On OK → CONTROL_PHASE_ROTATE
        - On PAUSED → [PAUSED]
        - On ERROR → [FAULT]

    CONTROL_PHASE_ROTATE:
        - ControlRotate_Service()
        - On OK → [DONE]
        - On PAUSED → [PAUSED]
        - On ERROR → [FAULT]

[PAUSED]
    - resume → return to previous phase
    - stop → [FAULT]

[DONE]
    - home / start allowed

[ESTOP]
    - latched until reset

[FAULT]
    - requires explicit recovery
```

---

## 5. File structure (updated to match actual repo)

```text
machine/
│
├── src/
│   ├── app/
│   │   ├── main.c              // main loop, ESTOP monitor
│   │   └── control.c           // state machine, orchestrator
│   │
│   ├── control/
│   │   ├── control_tilt.c      // non-blocking tilt engine
│   │   └── control_rotate.c    // non-blocking rotate engine
│   │
│   ├── calibration/
│   │   ├── calibration_tilt.c
│   │   └── calibration_rotate.c
│   │
│   ├── hal/
│   │   ├── motion.c            // mid-level motion helpers
│   │   ├── mio.c               // digital/analog I/O
│   │   ├── ro.c                // relay outputs
│   │   ├── tilt.c              // tilt-specific HAL
│   │   └── piControlIf.c       // RevPi interface
│   │
│   ├── include/
│   │   ├── control.h
│   │   ├── control_tilt.h
│   │   ├── control_rotate.h
│   │   ├── calibration_tilt.h
│   │   ├── calibration_rotate.h
│   │   ├── motion.h
│   │   ├── mio.h
│   │   ├── ro.h
│   │   └── tilt.h
│   │
│   └── config/
│       ├── mio_addr.h
│       ├── ro_addr.h
│       └── calibration_paths.h
│
├── data/
│   └── machine/
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

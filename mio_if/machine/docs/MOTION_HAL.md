# **MOTION HAL Documentation**  
### *Mid‑Level Motion I/O Abstraction*

---

## 1. Overview

The **Motion HAL** (`motion.c` / `motion.h`) provides a **machine‑specific, mid‑level abstraction** on top of the low‑level RevPi HAL:

- Digital I/O via **MIO HAL** (`mio.c`, `mio.h`)  
- Relay outputs via **RO HAL** (`ro.c`, `ro.h`)  
- Filtered analog tilt‑position measurement  

It exposes **semantic motion operations** instead of raw channels:

- E‑STOP input  
- Rotate home sensor  
- Tilt home sensor  
- Tilt position analog feedback (with filtering)  
- Rotate actuator control  
- Tilt actuator control  

This layer is designed for **state machines, IPC‑driven controllers, and autonomous routines**, keeping them free from raw channel numbers and process‑image details.

---

## 2. Architecture

```text
+--------------------------------------+
|  High-Level Logic                    |
|  (state machines, IPC, rider, etc.)  |
+--------------------+-----------------+
                     |
                     v
+--------------------------------------+
|  Motion HAL (motion.c / motion.h)    |
|  - ReadEStopButton                   |
|  - ReadHomeRotate / ReadHomeTilt     |
|  - ReadTiltPosition (3‑sample avg)   |
|  - RelayRotate / RelayTilt           |
+--------------------+-----------------+
                     |
                     v
+---------------------------+   +------------------------+
|  MIO HAL (mio.c / mio.h) |   |  RO HAL (ro.c / ro.h)  |
+---------------------------+   +------------------------+
                     |
                     v
+---------------------------+
|  piControl ioctl driver  |
+---------------------------+
```

**Key ideas:**

- High‑level code talks in **motion semantics**, not channels.  
- Motion HAL now provides **filtered analog tilt readings**.  
- Filtering is done inside Motion HAL so higher layers stay clean.  

---

## 3. File Structure

```text
machine/
│
├── src/
│   ├── hal/
│   │   ├── mio.c
│   │   ├── ro.c
│   │   └── piControlIf.c
│   │
│   ├── include/
│   │   ├── mio.h
│   │   ├── ro.h
│   │   └── motion.h
│   │
│   ├── config/
│   │   ├── mio_addr.h
│   │   └── ro_addr.h
│   │
│   └── motion.c
│
└── test/
    ├── test_mio.c
    ├── test_ro.c
    └── test_motion_hw.c
```

---

## 4. Channel Mapping

These symbolic names in `motion.h` map **machine semantics** to **MIO/RO channels**.

### 4.1 Digital Inputs (`mio_get_di`)

| Symbol             | Channel | Description                     |
|-------------------|---------|---------------------------------|
| `DI_PROXI_ROTATE` | 1       | Rotate‑axis home sensor         |
| `DI_PROXI_TILT`   | 2       | Tilt‑axis home sensor           |
| `DI_ESTOP`        | 4       | Emergency stop button           |

### 4.2 Analog Inputs (`mio_get_ai`)

| Symbol        | Channel | Description               |
|---------------|---------|---------------------------|
| `AI_TILT_POS` | 1       | Tilt position feedback    |

### 4.3 Relay Outputs (`ro_set_ro`)

| Symbol          | Channel | Meaning                                |
|-----------------|---------|-----------------------------------------|
| `RO_TILT_DIR`   | 4       | Tilt direction (`1`=up, `0`=down)      |
| `RO_TILT_EN`    | 3       | Tilt enable                            |
| `RO_ROTATE_DIR` | 2       | Rotate direction (`1`=cw, `0`=ccw)     |
| `RO_ROTATE_EN`  | 1       | Rotate enable                          |

These mappings are **machine‑specific** and must match the wiring and `.rsc` configuration.

---

## 5. API Reference

All functions are declared in `motion.h` and implemented in `motion.c`.

---

### 5.1 Read Functions

#### `int ReadEStopButton(void);`

Reads the emergency stop button.

**Returns:**

- `1` — pressed  
- `0` — not pressed  
- `-1` — read error  

---

#### `int ReadHomeRotate(void);`

Reads the rotate‑axis home sensor.

**Returns:**

- `1` — at home  
- `0` — not at home  
- `-1` — read error  

---

#### `int ReadHomeTilt(void);`

Reads the tilt‑axis home sensor.

**Returns:**

- `1` — at home  
- `0` — not at home  
- `-1` — read error  

---

### `int ReadTiltPosition(void);`  
**Filtered analog tilt‑position measurement**

This function reads the tilt position analog input using a **3‑sample filtered average**:

- Reads the ADC **three times**  
- Waits **1 ms** between samples  
- Returns the **integer average**  

This provides a stable reading with minimal latency.

#### **ADC Characteristics**

- ADC range: **0–10000** → **0–10 V**  
- ADC cannot reliably measure below **~25 counts (~0.025 V)**  
- **Note:** The linear actuator’s meaningful range is:  
  - **0.95 V → 0°** (≈950 counts)  
  - **9.23 V → 90°** (≈9230 counts)  
- Values below ~950 counts are **outside the actuator’s valid range**  

**Returns:**

- `0–10000` — filtered ADC value  
- `-1` — read error  

---

## 5.2 Relay Control Functions

#### `void RelayRotate(int cw, int on);`

Controls the rotate actuator.

**Parameters:**

- `cw` — `1` = clockwise, `0` = counterclockwise  
- `on` — `1` = enable rotation, `0` = disable rotation  

**Behavior:**

- When `on == 1`  
  - Sets `RO_ROTATE_DIR` based on `cw`  
  - Sets `RO_ROTATE_EN = 1`  
- When `on == 0`  
  - Sets `RO_ROTATE_EN = 0`  
  - Resets `RO_ROTATE_DIR = 0`  

---

#### `void RelayTilt(int up, int on);`

Controls the tilt actuator.

**Parameters:**

- `up` — `1` = tilt up (toward 0°), `0` = tilt down (toward home)  
- `on` — `1` = enable tilt, `0` = disable tilt  

**Behavior:**

- When `on == 1`  
  - Sets `RO_TILT_DIR` based on `up`  
  - Sets `RO_TILT_EN = 1`  
- When `on == 0`  
  - Sets `RO_TILT_EN = 0`  
  - Resets `RO_TILT_DIR = 0`  

---

## 6. Hardware Test Program — `test_motion_hw.c`

`test_motion_hw.c` validates that the **Motion HAL** correctly drives real hardware via MIO and RO.

### 6.1 What it tests

- **Inputs:**
  - E‑STOP DI range (`0` or `1`)  
  - Rotate home DI range (`0` or `1`)  
  - Tilt home DI range (`0` or `1`)  
  - Tilt position AI range (`0–10000`)  

- **Rotate actuator:**
  - `RelayRotate(1, 1)` → `RO_ROTATE_EN == 1`  
  - `RelayRotate(1, 0)` → `RO_ROTATE_EN == 0`  
  - `RelayRotate(0, 1)` → `RO_ROTATE_EN == 1`  
  - `RelayRotate(0, 0)` → `RO_ROTATE_EN == 0`  

- **Tilt actuator:**
  - `RelayTilt(1, 1)` → `RO_TILT_EN == 1`  
  - `RelayTilt(1, 0)` → `RO_TILT_EN == 0`  
  - `RelayTilt(0, 1)` → `RO_TILT_EN == 1`  
  - `RelayTilt(0, 0)` → `RO_TILT_EN == 0`  

### 6.2 Example Output

```text
=== Initializing HAL ===

=== Hardware Test: Inputs ===
E-STOP (expect 0 or 1)      : PASS
Home Rotate (0 or 1)        : PASS
Home Tilt (0 or 1)          : PASS
Tilt Position (0–10000)     : PASS

=== Hardware Test: Rotate ===
Rotate EN=1                 : PASS
Rotate EN=0                 : PASS
Rotate EN=1                 : PASS
Rotate EN=0                 : PASS

=== Hardware Test: Tilt ===
Tilt EN=1                   : PASS
Tilt EN=0                   : PASS
Tilt EN=1                   : PASS
Tilt EN=0                   : PASS

=== Hardware Validation Complete ===
```

---

## 7. Error Handling

The Motion HAL simply forwards error semantics from the underlying HAL:

- All read functions return `-1` on error.  
- Relay functions do not return a status; errors must be detected via `ro_get_ro()` or lower‑level diagnostics.

Common causes:

- `mio_init()` or `ro_init()` not called  
- piControl driver not loaded  
- Wrong `.rsc` configuration  
- Hardware not connected or miswired  

High‑level logic should treat `-1` as **“sensor invalid / hardware fault”**.

---

## 8. Best Practices

- Use Motion HAL as the **only** interface for motion‑related I/O.  
- Always use `ReadTiltPosition()` instead of raw ADC reads.  
- Treat values below ~950 counts as “0°” or invalid.  
- Use `test_motion_hw.c` after wiring or hardware changes.  
- Consider adding higher‑level safety wrappers (e.g., auto‑disable relays on ESTOP).

---

## 9. Purpose and Role in the System

The Motion HAL:

- Shields high‑level logic from raw I/O details  
- Provides **stable, filtered tilt measurements**  
- Encodes machine semantics in one place  
- Supports deterministic, maintainable motion control  

It sits at the sweet spot between **hardware‑specific** and **application‑specific**, making your system easier to reason about, test, and evolve.

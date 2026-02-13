# **MIO HAL Documentation**  
### *RevPi MIO Hardware Abstraction Layer*

---

## 1. Overview

The **MIO HAL** provides a clean, channel‑based API for interacting with the **Revolution Pi MIO module**.  
It abstracts the low‑level `piControl` ioctl interface into simple, deterministic functions for:

- Digital Inputs (DI1–DI4)
- Digital Outputs (DO1–DO4)
- Analog Inputs (AI1–AI8)
- Analog Outputs (AO1–AO8)

This HAL is intentionally minimal, stable, and predictable — ideal for deterministic machine control.

---

## 2. Architecture

```
+---------------------------+
|  High-Level Logic         |
|  (motion.c, rider, etc.) |
+-------------+-------------+
              |
              v
+---------------------------+
|  MIO HAL (mio.c / mio.h) |
+-------------+-------------+
              |
              v
+---------------------------+
|  piControl ioctl driver  |
+---------------------------+
```

### Key design principles

- **No mmap** — ioctl only (safe, simple, deterministic)  
- **Channel‑based API** — no offsets exposed to application code  
- **Source‑validated offsets** — defined in `src/config/mio_addr.h`  
- **Error‑aware** — all functions return `-1` on failure  

---

## 3. File Structure

```
machine/
│
├── src/
│   ├── hal/
│   │   ├── mio.c
│   │   ├── piControlIf.c
│   │   └── ...
│   │
│   ├── include/
│   │   ├── mio.h
│   │   └── ...
│   │
│   ├── config/
│   │   ├── mio_addr.h        ← Digital/Analog offsets & bit positions
│   │   └── ro_addr.h
│   │
│   └── utils/
│
└── test/
    └── test_mio.c
```

---

## 4. Initialization

### `int mio_init(void);`

Opens the piControl device.  
Must be called **once** before any other MIO function.

**Returns:**

| Value | Meaning |
|-------|---------|
| `0` | Success |
| `-1` | Failed to open piControl |

---

## 5. Digital I/O API

### `int mio_get_di(int ch);`

Reads a digital input channel.

| Parameter | Meaning |
|----------|---------|
| `ch` | DI channel (1–4) |

**Returns:**

- `0` — input low  
- `1` — input high  
- `-1` — invalid channel or read error  

---

### `int mio_get_do(int ch);`

Reads a digital output channel.

**Returns:** same as above.

---

### `int mio_set_do(int ch, int value);`

Sets a digital output channel.

| Parameter | Meaning |
|----------|---------|
| `ch` | DO channel (1–4) |
| `value` | `0` = OFF, `1` = ON |

**Returns:**

- `0` — success  
- `-1` — invalid channel or write error  

---

## 6. Analog I/O API

### `int mio_get_ai(int ch);`

Reads an analog input channel.

| Parameter | Meaning |
|----------|---------|
| `ch` | AI channel (1–8) |

**Returns:**

- `0–10000` — raw 0–10 V value  
- `-1` — invalid channel or read error  

---

### `int mio_get_ao(int ch);`

Reads an analog output channel.

**Returns:** same as above.

---

### `int mio_set_ao(int ch, uint16_t value);`

Sets an analog output channel.

| Parameter | Meaning |
|----------|---------|
| `value` | 0–10000 raw (0–10 V) |

**Returns:**

- `0` — success  
- `-1` — error  

---

## 7. Channel Mapping (from `src/config/mio_addr.h`)

All offsets and bit positions are defined in:

```
src/config/mio_addr.h
```

Examples:

```
DI1_OFFSET, DI1_BIT
DO3_OFFSET, DO3_BIT
AI1_OFFSET
AO1_OFFSET
```

These values come directly from the RevPi process image and must match the `.rsc` configuration.

---

## 8. Test Program — `test_mio.c`

This test validates:

- Digital input reading  
- Digital output switching  
- Analog input reading  
- Analog output voltage setting  
- Timing and tolerance behavior  

### **Hardware Assumptions (Required for the Test)**

The test relies on two physical loopback connections:

- **DO3 → DI1 are hard‑wired**  
  Used to verify digital output → digital input propagation.

- **AO1 → AI1 are hard‑wired**  
  Used to verify analog output → analog input voltage tracking.

These connections must be present for the test to pass.

### Features

- Measures propagation delay (µs resolution)  
- Validates **DO3 → DI1** digital loopback  
- Validates **AO1 → AI1** analog loopback  
- Uses tolerance windows for analog settling  
- Prints warnings on timeout  

### Example Output

```
=== MIO HAL TEST ===
DI1 = 0

DO3 <-- 1
DO3 = 1 (after 120 us)

AI1 = 5000 (5.000V)
AI1 = 10000 (10.000V)
AI1 = 0 (0.000V)
```

### How to build

```
make test_mio
```

### How to run

```
./build/test_mio
```

---

## 9. Error Handling

All MIO functions return `-1` on error.  
Common causes:

- piControl not loaded  
- Invalid channel number  
- Hardware not connected  
- Wrong `.rsc` configuration  

Your application should always check return values.

---

## 10. Best Practices

- Call `mio_init()` once at startup  
- Never assume DI/AI values — always check for `-1`  
- Use tolerance windows for analog validation  
- Keep all channel numbers symbolic (never hard‑code)  
- Use `test_mio.c` after wiring changes or hardware replacement  

---

## 11. Summary

The MIO HAL provides:

- A clean, deterministic interface to RevPi MIO hardware  
- A stable API for digital and analog I/O  
- A hardware validation test suite  
- A safe abstraction layer for higher‑level machine logic  

It is designed to be:

- Maintainable  
- Predictable  
- Easy to test  
- Industrial‑grade  

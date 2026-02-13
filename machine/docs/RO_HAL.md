# **RO HAL Documentation**  
### *RevPi RO Hardware Abstraction Layer*

---

## 1. Overview

The **RO HAL** provides a deterministic, channel‑based API for controlling the **Relay Output (RO)** module on the Revolution Pi.  
It abstracts the low‑level `piControl` ioctl interface into simple functions for:

- Reading relay output states  
- Writing relay output states  
- Retrieving process‑image mapping (offset, bit, length)

The RO module contains **4 relay outputs**, each mapped to a single bit in the process image.  
This HAL ensures that higher‑level logic (e.g., `motion.c`) never touches offsets or bit positions directly.

---

## 2. Architecture

```
+---------------------------+
|  High-Level Logic         |
|  (motion.c, state machine)|
+-------------+-------------+
              |
              v
+---------------------------+
|  RO HAL (ro.c / ro.h)    |
+-------------+-------------+
              |
              v
+---------------------------+
|  piControl ioctl driver  |
+---------------------------+
```

### Design Principles

- **No mmap** — ioctl only  
- **Channel‑based API** — no offsets exposed to application code  
- **Source‑validated offsets** — defined in `src/config/ro_addr.h`  
- **Error‑aware** — all functions return `-1` on failure  
- **Deterministic** — relay writes are atomic and predictable  

---

## 3. File Structure

```
machine/
│
├── src/
│   ├── hal/
│   │   ├── ro.c
│   │   ├── mio.c
│   │   ├── piControlIf.c
│   │   └── ...
│   │
│   ├── include/
│   │   ├── ro.h
│   │   ├── mio.h
│   │   ├── piControl.h
│   │   ├── piControlIf.h
│   │   └── ...
│   │
│   ├── config/
│   │   ├── ro_addr.h        ← **Relay Output offsets & bit positions**
│   │   ├── mio_addr.h
│   │   └── ...
│   │
│   └── utils/
│
└── test/
    ├── test_ro.c
    ├── test_mio.c
    └── ...
```

---

## 4. Initialization

### `int ro_init(void);`

Opens the piControl device.  
Must be called **once** before any RO function.

**Returns:**

| Value | Meaning |
|-------|---------|
| `0` | Success |
| `-1` | Failed to open piControl |

---

## 5. Relay Output API

### `int ro_get_ro(int ch);`

Reads the current state of a relay output.

| Parameter | Meaning |
|----------|---------|
| `ch` | Relay channel (1–4) |

**Returns:**

- `0` — relay OFF  
- `1` — relay ON  
- `-1` — invalid channel or read error  

---

### `int ro_set_ro(int ch, int value);`

Sets the state of a relay output.

| Parameter | Meaning |
|----------|---------|
| `ch` | Relay channel (1–4) |
| `value` | `0` = OFF, `1` = ON |

**Returns:**

- `0` — success  
- `-1` — invalid channel or write error  

---

### `int ro_get_addr(int ch, int *offset, int *bit, int *len);`

Retrieves the process‑image mapping for a relay channel.

| Output | Meaning |
|--------|---------|
| `offset` | Byte offset in process image |
| `bit` | Bit position (0–3) |
| `len` | Bit length (always 1) |

**Returns:**

- `0` — success  
- `-1` — invalid channel or null pointer  

This function is used internally by test programs and debugging tools.

---

## 6. Channel Mapping (from `src/config/ro_addr.h`)

The RO module uses a **single output byte** at offset **75**, with bits **0–3** mapped to RO1–RO4.

| Relay Output | Offset | Bit | Length |
|--------------|--------|-----|--------|
| RO1 | 75 | 0 | 1 |
| RO2 | 75 | 1 | 1 |
| RO3 | 75 | 2 | 1 |
| RO4 | 75 | 3 | 1 |

These values were validated using:

- `piTest -d`  
- `piTest -v RelayOutput_*`  
- `dmesg | grep -i picontrol`  
- C test programs (`test_ro.c`)  

---

## 7. Test Program — `test_ro.c`

This test validates:

- Relay output readback  
- Relay output write operations  
- Correct offset/bit mapping  
- Real hardware switching behavior  

### Features

- Prints all RO channel mappings  
- Toggles each relay ON → OFF  
- Reads back state after each write  
- Includes delays for physical relay settling  

### Example Output

```
====== RO Module ======

=== Relay Outputs ===
RO1: offset=75 bit=0 len=1
RO2: offset=75 bit=1 len=1
RO3: offset=75 bit=2 len=1
RO4: offset=75 bit=3 len=1

-- Testing RO1 --
Initial state: 0
Setting RO1 = 1...
Readback: 1
Setting RO1 = 0...
Readback: 0
```

### Build

```
make test_ro
```

### Run

```
./build/test_ro
```

---

## 8. Error Handling

All RO functions return `-1` on error.  
Common causes:

- piControl not loaded  
- Invalid channel number  
- Hardware not connected  
- Wrong `.rsc` configuration  

Your application should always check return values.

---

## 9. Best Practices

- Call `ro_init()` once at startup  
- Never assume relay state — always read back if needed  
- Use symbolic channel names (never hard‑code numbers)  
- Use `test_ro.c` after wiring changes or hardware replacement  
- Keep relay writes deterministic (direction before enable, etc.)  

---

## 10. Summary

The RO HAL provides:

- A clean, deterministic interface to RevPi relay outputs  
- A stable API for reading and writing RO1–RO4  
- A hardware validation test suite  
- A safe abstraction layer for higher‑level motion logic  

It is designed to be:

- Maintainable  
- Predictable  
- Easy to test  
- Industrial‑grade  

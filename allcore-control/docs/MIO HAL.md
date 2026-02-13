# MIO HAL — RevPi MIO Hardware Abstraction Layer

## Overview

The **MIO HAL** provides a channel-based API for interacting with the
Revolution Pi MIO module. It abstracts the low-level piControl ioctl interface
into simple functions for reading and writing digital and analog I/O.

---

## Features

### Digital I/O
- 4 Digital Inputs (DI1–DI4)
- 4 Digital Outputs (DO1–DO4)

### Analog I/O
- 8 Analog Inputs (AI1–AI8)
- 8 Analog Outputs (AO1–AO8)
- 0–10 V range mapped to 0–10000 raw counts

### Architecture
- No `mmap` used (ioctl only)
- Offsets defined in `mio_addr.h`
- Public API in `mio.h`
- Implementation in `mio.c`

---

## Initialization

```c
int mio_init(void);


# IPC architecture diagram 
# (UNIX domain datagram sockets)

```
                         (AF_UNIX, SOCK_DGRAM)

 ┌──────────────────────┐
 │    MonitorEStop      │
 │──────────────────────│
 │ - polls DI1 (100 ms) │
 │ - on DI1==1 → sends  │
 │   MSG_ESTOP_PRESSED  │
 └───────────┬──────────┘
             │
             │  /tmp/estop.sock
             ▼
 ┌──────────────────────┐        /tmp/rotate.sock        ┌──────────────────────┐
 │        Rider         │────────────────────────────────>│        Rotate        │
 │──────────────────────│   MSG_ROTATE_START / STOP       │──────────────────────│
 │ - binds:             │                                 │ - binds:             │
 │   /tmp/rider.sock    │                                 │   /tmp/rotate.sock   │
 │   /tmp/estop.sock    │                                 │                      │
 │                      │<────────────────────────────────┤                      │
 │ - receives:          │        MSG_ROTATE_DONE          │ - sends DONE to      │
 │   MSG_ESTOP_PRESSED  │        /tmp/rider.sock          │   /tmp/rider.sock    │
 │   MSG_ROTATE_DONE    │                                 │                      │
 │                      │                                 │ - stops on STOP      │
 │ - sends:             │                                 │                      │
 │   MSG_ROTATE_START   │                                 │                      │
 │   MSG_ROTATE_STOP    │                                 │                      │
 └──────────────────────┘                                 └──────────────────────┘
```

---

## Message Flow
### MonitorEStop → Rider
- Sends: MSG_ESTOP_PRESSED
- Socket: /tmp/estop.sock

### Rider → Rotate
- Sends: MSG_ROTATE_START, MSG_ROTATE_STOP
- Socket: /tmp/rotate.sock

### Rotate → Rider
- Sends: MSG_ROTATE_DONE
- Socket: /tmp/rider.sock

---

## Build & Run

### Build
```
gcc -Wall -Wextra -O2 -g -o rotate rotate.c
gcc -Wall -Wextra -O2 -g -o monitor_estop monitor_estop.c
gcc -Wall -Wextra -O2 -g -o rider rider.c
```

### Run
```
./monitor_estop      # terminal 1
./rider              # terminal 2 (spawns ./rotate)
# press ENTER in monitor_estop to simulate ESTOP
```

---
---

# 🔧 **New Required Behavior**

## **Rotate (worker)**  
- Waits for `START`  
- Runs a **1‑minute timer**  
- While running:
  - Accepts: `PAUSE`, `RESUME`, `STOP`
  - If `PAUSE`: stop timer, stop motion  
  - If `RESUME`: continue timer  
  - If `STOP`: exit immediately  
- After 1 minute of *actual running time*, send `DONE` to Rider  
- Exit

---

## **Rider (master)**  
- Sends `START` to Rotate  
- After **10 seconds**, sends `PAUSE`  
- After **5 seconds**, sends `RESUME`  
- If Rider receives `ESTOP`:
  - Sends `STOP` to Rotate  
  - **Does NOT exit**  
  - Continues running (waiting for next commands or events)

---

## **New Message Types Needed**

Add these to `ipc.h`:

```
MSG_ROTATE_PAUSE,
MSG_ROTATE_RESUME,
```

---

# ✅ **Updated ipc.h**

---

# ✅ **Updated rotate.c (full file)**  
Implements: START → PAUSE → RESUME → STOP → DONE after 60s running time.

---

# ✅ **Updated rider.c (full file)**  
Implements:  
- START → wait 10s → PAUSE → wait 5s → RESUME  
- If ESTOP → send STOP but **do not exit**  
- Continue running normally

---

# 🎉 **Your IPC system now supports:**

- START  
- PAUSE  
- RESUME  
- STOP  
- DONE  
- ESTOP (asynchronous, non‑terminating for Rider)

This is now a proper **PLC‑style master/worker** control loop.

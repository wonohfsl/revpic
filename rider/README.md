
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

### Result
```
root@24353d6af54d:/project/dev/rider2# ./rider
[Rider] Started Rotate (pid=33)
[Rotate] Waiting for START...
[Rider] Sent START.
[Rotate] START received.
[Rider] Sent PAUSE.
[Rotate] PAUSE received.
[Rider] Sent RESUME.
[Rotate] RESUME received.
[Rotate] Completed 60 seconds. Sending DONE.
[Rider] Rotate DONE received.
[Rotate] Exiting.
[Rider] Rotate exited with status 0
[Rider] ESTOP received! Sending STOP to Rotate.
[Rider] sendto STOP: No such file or directory
```
Rotate process is exited after it reached 60 seconds.
No zombie process (./rotate) left behind.
```
root@24353d6af54d:/# ps aux
USER         PID %CPU %MEM    VSZ   RSS TTY      STAT START   TIME COMMAND
root           1  0.0  0.0   4036  2944 pts/0    Ss+  23:22   0:00 /bin/bash
root           9  0.1  0.0   4036  3072 pts/1    Ss   23:27   0:00 /bin/bash
root          32  0.0  0.0   2196  1152 pts/1    S+   23:27   0:00 ./rider
root          34  0.2  0.0   4036  3072 pts/2    Ss   23:28   0:00 /bin/bash
root          42  0.3  0.0   4036  2944 pts/3    Ss   23:28   0:00 /bin/bash
root          50  0.0  0.0   2192   896 pts/3    S+   23:29   0:00 ./monitor_estop
root          51  0.0  0.0   8044  3712 pts/2    R+   23:29   0:00 ps aux
```
#ifndef IPC_H
#define IPC_H

#include <sys/socket.h>
#include <sys/un.h>

#define SOCK_ROTATE_PATH "/tmp/rotate.sock"
#define SOCK_RIDER_PATH  "/tmp/rider.sock"
#define SOCK_ESTOP_PATH  "/tmp/estop.sock"

typedef enum {
    MSG_ROTATE_START = 1,
    MSG_ROTATE_STOP,
    MSG_ROTATE_DONE,
    MSG_ROTATE_PAUSE,
    MSG_ROTATE_RESUME,
    MSG_ESTOP_PRESSED
} msg_type_t;

typedef struct {
    msg_type_t type;
} msg_t;

#endif /* IPC_H */

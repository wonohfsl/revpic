#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>
#include <errno.h>

#include "ipc.h"

/* Replace with real hardware read, e.g., mio_get_di(1) */
static int read_estop_input(void)
{
    static int initialized = 0;
    static int last_state = 0;

    if (!initialized) {
        int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
        fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
        initialized = 1;
    }

    char buf[8];
    ssize_t n = read(STDIN_FILENO, buf, sizeof(buf));
    if (n > 0) {
        last_state = 1; /* simulate ESTOP pressed */
    }
    return last_state;
}

int main(void)
{
    int sock = -1;
    struct sockaddr_un rider_addr;
    msg_t msg;

    sock = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("socket");
        return 1;
    }

    memset(&rider_addr, 0, sizeof(rider_addr));
    rider_addr.sun_family = AF_UNIX;
    strncpy(rider_addr.sun_path, SOCK_ESTOP_PATH,
            sizeof(rider_addr.sun_path) - 1);

    printf("[MonitorEStop] Running. Press ENTER to simulate ESTOP.\n");

    int last = 0;

    while (1) {
        int cur = read_estop_input();

        if (cur == 1 && last == 0) {
            printf("[MonitorEStop] ESTOP pressed! Sending message.\n");
            msg.type = MSG_ESTOP_PRESSED;
            if (sendto(sock, &msg, sizeof(msg), 0,
                       (struct sockaddr *)&rider_addr,
                       sizeof(rider_addr)) < 0) {
                perror("[MonitorEStop] sendto");
            }
        }

        last = cur;
        usleep(100000); /* 100 ms */
    }

    close(sock);
    return 0;
}

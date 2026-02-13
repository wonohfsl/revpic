#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>
#include <errno.h>

#include "ipc.h"

static long long now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000LL + ts.tv_nsec / 1000000;
}

int main(void)
{
    int sock = -1;
    struct sockaddr_un addr;
    msg_t msg;

    sock = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("socket");
        return 1;
    }

    unlink(SOCK_ROTATE_PATH);
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCK_ROTATE_PATH, sizeof(addr.sun_path) - 1);

    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind rotate");
        close(sock);
        return 1;
    }

    printf("[Rotate] Waiting for START...\n");

    struct sockaddr_un src_addr;
    socklen_t src_len = sizeof(src_addr);
    ssize_t n = recvfrom(sock, &msg, sizeof(msg), 0,
                         (struct sockaddr *)&src_addr, &src_len);
    if (n < 0) {
        perror("recvfrom START");
        close(sock);
        return 1;
    }

    if (msg.type != MSG_ROTATE_START) {
        printf("[Rotate] Unexpected first message: %d\n", msg.type);
        close(sock);
        return 1;
    }

    printf("[Rotate] START received. Running for 60 seconds...\n");

    int running = 1;
    long long start_ms = now_ms();

    while (running) {
        /* Non-blocking check for STOP */
        struct timeval tv = {0, 0};
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(sock, &rfds);

        int ret = select(sock + 1, &rfds, NULL, NULL, &tv);
        if (ret > 0 && FD_ISSET(sock, &rfds)) {
            n = recvfrom(sock, &msg, sizeof(msg), 0,
                         (struct sockaddr *)&src_addr, &src_len);
            if (n > 0 && msg.type == MSG_ROTATE_STOP) {
                printf("[Rotate] STOP received. Exiting.\n");
                running = 0;
                break;
            }
        } else if (ret < 0 && errno != EINTR) {
            perror("[Rotate] select");
        }

        long long elapsed_ms = now_ms() - start_ms;
        if (elapsed_ms >= 60000) { /* 60 seconds */
            printf("[Rotate] Completed 60 seconds. Sending DONE.\n");

            int sock_out = socket(AF_UNIX, SOCK_DGRAM, 0);
            if (sock_out < 0) {
                perror("[Rotate] socket out");
            } else {
                struct sockaddr_un rider_addr;
                memset(&rider_addr, 0, sizeof(rider_addr));
                rider_addr.sun_family = AF_UNIX;
                strncpy(rider_addr.sun_path, SOCK_RIDER_PATH,
                        sizeof(rider_addr.sun_path) - 1);

                msg.type = MSG_ROTATE_DONE;
                if (sendto(sock_out, &msg, sizeof(msg), 0,
                           (struct sockaddr *)&rider_addr,
                           sizeof(rider_addr)) < 0) {
                    perror("[Rotate] sendto DONE");
                }
                close(sock_out);
            }

            running = 0;
            break;
        }

        usleep(100000); /* 100 ms tick */
    }

    close(sock);
    unlink(SOCK_ROTATE_PATH);

    printf("[Rotate] Exiting.\n");
    return 0;
}

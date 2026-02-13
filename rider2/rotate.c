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
    int sock = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("socket");
        return 1;
    }

    unlink(SOCK_ROTATE_PATH);

    struct sockaddr_un addr;
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
    msg_t msg;

    int started = 0;
    int paused = 0;
    int running = 1;
    long long accumulated_ms = 0;
    long long last_tick = now_ms();

    while (running) {
        struct timeval tv = {0, 100000}; /* 100 ms timeout */
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(sock, &rfds);

        int ret = select(sock + 1, &rfds, NULL, NULL, &tv);
        if (ret > 0 && FD_ISSET(sock, &rfds)) {
            ssize_t n = recvfrom(sock, &msg, sizeof(msg), 0,
                                 (struct sockaddr *)&src_addr, &src_len);
            if (n > 0) {
                if (!started) {
                    if (msg.type == MSG_ROTATE_START) {
                        printf("[Rotate] START received.\n");
                        started = 1;
                        last_tick = now_ms();
                    } else {
                        continue; /* ignore anything before START */
                    }
                } else {
                    if (msg.type == MSG_ROTATE_STOP) {
                        printf("[Rotate] STOP received. Exiting.\n");
                        running = 0;
                        break;
                    } else if (msg.type == MSG_ROTATE_PAUSE) {
                        printf("[Rotate] PAUSE received.\n");
                        paused = 1;
                    } else if (msg.type == MSG_ROTATE_RESUME) {
                        printf("[Rotate] RESUME received.\n");
                        paused = 0;
                        last_tick = now_ms();
                    }
                }
            }
        } else if (ret < 0 && errno != EINTR) {
            perror("[Rotate] select");
        }

        if (started && !paused) {
            long long now = now_ms();
            accumulated_ms += (now - last_tick);
            last_tick = now;

            if (accumulated_ms >= 60000) {
                printf("[Rotate] Completed 60 seconds. Sending DONE.\n");

                int sock_out = socket(AF_UNIX, SOCK_DGRAM, 0);
                if (sock_out >= 0) {
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
        }

        /* loop tick already controlled by select timeout */
    }

    close(sock);
    unlink(SOCK_ROTATE_PATH);

    printf("[Rotate] Exiting.\n");
    return 0;
}

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>

#include "ipc.h"

static int bind_unix_dgram(const char *path)
{
    int sock = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("socket");
        return -1;
    }

    unlink(path);

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);

    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(sock);
        return -1;
    }

    return sock;
}

int main(void)
{
    int sock_rider = -1;
    int sock_estop = -1;
    msg_t msg;

    sock_rider = bind_unix_dgram(SOCK_RIDER_PATH);
    if (sock_rider < 0) {
        return 1;
    }

    sock_estop = bind_unix_dgram(SOCK_ESTOP_PATH);
    if (sock_estop < 0) {
        close(sock_rider);
        unlink(SOCK_RIDER_PATH);
        return 1;
    }

    /* Start Rotate process */
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return 1;
    } else if (pid == 0) {
        execlp("./rotate", "./rotate", NULL);
        perror("execlp rotate");
        _exit(1);
    }

    printf("[Rider] Started Rotate (pid=%d)\n", pid);

    /* Send START to Rotate */
    int sock_cmd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (sock_cmd < 0) {
        perror("[Rider] socket cmd");
        return 1;
    }

    struct sockaddr_un rotate_addr;
    memset(&rotate_addr, 0, sizeof(rotate_addr));
    rotate_addr.sun_family = AF_UNIX;
    strncpy(rotate_addr.sun_path, SOCK_ROTATE_PATH,
            sizeof(rotate_addr.sun_path) - 1);

    msg.type = MSG_ROTATE_START;
    if (sendto(sock_cmd, &msg, sizeof(msg), 0,
               (struct sockaddr *)&rotate_addr,
               sizeof(rotate_addr)) < 0) {
        perror("[Rider] sendto START");
    } else {
        printf("[Rider] Sent START to Rotate.\n");
    }

    int done = 0;
    int estop_triggered = 0;

    while (!done) {
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(sock_rider, &rfds);
        FD_SET(sock_estop, &rfds);
        int maxfd = (sock_rider > sock_estop ? sock_rider : sock_estop) + 1;

        int ret = select(maxfd, &rfds, NULL, NULL, NULL);
        if (ret < 0) {
            if (errno == EINTR) continue;
            perror("[Rider] select");
            break;
        }

        if (FD_ISSET(sock_rider, &rfds)) {
            ssize_t n = recvfrom(sock_rider, &msg, sizeof(msg), 0, NULL, NULL);
            if (n > 0 && msg.type == MSG_ROTATE_DONE) {
                printf("[Rider] Rotate DONE received.\n");
                done = 1;
            }
        }

        if (FD_ISSET(sock_estop, &rfds)) {
            ssize_t n = recvfrom(sock_estop, &msg, sizeof(msg), 0, NULL, NULL);
            if (n > 0 && msg.type == MSG_ESTOP_PRESSED) {
                printf("[Rider] ESTOP received! Sending STOP to Rotate.\n");
                estop_triggered = 1;
                msg.type = MSG_ROTATE_STOP;
                if (sendto(sock_cmd, &msg, sizeof(msg), 0,
                           (struct sockaddr *)&rotate_addr,
                           sizeof(rotate_addr)) < 0) {
                    perror("[Rider] sendto STOP");
                }
                done = 1;
            }
        }
    }

    printf("[Rider] Waiting for Rotate to exit...\n");
    int status = 0;
    waitpid(pid, &status, 0);
    printf("[Rider] Rotate exited with status %d (estop=%d)\n", status, estop_triggered);

    close(sock_cmd);
    close(sock_rider);
    close(sock_estop);

    unlink(SOCK_RIDER_PATH);
    unlink(SOCK_ESTOP_PATH);

    return 0;
}

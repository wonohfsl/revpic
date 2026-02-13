#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

#include "dio.h"

int main(void)
{
    int fd = open("/dev/piControl0", O_RDWR);

    int v;

    dio_get(fd, 1, &v);
    printf("DI1 = %d\n", v);

    dio_set(fd, 3, 1, 50000);
    dio_get(fd, 1, &v);
    printf("DI1 = %d\n", v);

    dio_set(fd, 3, 0, 50000);
    dio_get(fd, 1, &v);
    printf("DI1 = %d\n", v);

    close(fd);
}

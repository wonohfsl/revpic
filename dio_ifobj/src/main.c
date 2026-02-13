#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

#include "dio.h"

int main(void)
{
    int fd = open("/dev/piControl0", O_RDWR);

    dio_t dio = dio_create(fd);

    int v;

    dio.get(&dio, 1, &v);
    printf("DI1 = %d\n", v);

    dio.set(&dio, 3, 1, 50000);
    dio.get(&dio, 1, &v);
	printf("DI1 = %d\n", v);

    dio.set(&dio, 3, 0, 50000);
    dio.get(&dio, 1, &v);
	printf("DI1 = %d\n", v);

    close(fd);
}

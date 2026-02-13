#ifndef DIO_H
#define DIO_H

#include <stdint.h>

int dio_get(int fd, int channel, int *value);
int dio_set(int fd, int channel, int value, long long timeout_us);

#endif

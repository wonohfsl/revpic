#ifndef DIO_H
#define DIO_H

typedef struct dio_s {
    int fd;

    int (*get)(struct dio_s *self, int channel, int *value);
    int (*set)(struct dio_s *self, int channel, int value, long long timeout_us);

} dio_t;

dio_t dio_create(int fd);

#endif

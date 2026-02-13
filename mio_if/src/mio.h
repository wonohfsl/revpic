#ifndef MIO_H
#define MIO_H

#include <stdint.h>

/* Initialize piControl */
int mio_init(void);

/* Digital Input (DI1–DI4) */
int mio_get_di(int ch);

/* Digital Output (DO1–DO4) */
int mio_set_do(int ch, int value);
int mio_get_do(int ch);

/* Analog Input (AI1–AI8) */
int mio_get_ai(int ch);

/* Analog Output (AO1–AO8) */
int mio_get_ao(int ch);
int mio_set_ao(int ch, uint16_t value);

#endif

#ifndef HAL_H

#define HAL_H
#include <stdint.h>

int  hal_init(void);
void hal_close(void);

uint8_t hal_read_byte(uint16_t offset);
void    hal_write_byte(uint16_t offset, uint8_t value);

#endif

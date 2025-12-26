#include "hal.h"
#include <unistd.h>
#include <stdio.h>

// Forward declarations
int  hal_revpi_init(void);
void hal_revpi_close(void);

uint8_t hal_revpi_read_byte(uint16_t offset);
void    hal_revpi_write_byte(uint16_t offset, uint8_t value);

int  hal_sim_init(void);
void hal_sim_close(void);

uint8_t hal_sim_read_byte(uint16_t offset);
void    hal_sim_write_byte(uint16_t offset, uint8_t value);

static int use_sim = 1;

int hal_init(void) {
    if (access("/dev/piControl0", F_OK) == 0) {
        if (hal_revpi_init() == 0) {
            use_sim = 0;
            printf("HAL: using RevPi backend\n");
            return 0;
        }
    }
    if (hal_sim_init() == 0) {
        use_sim = 1;
        printf("HAL: using simulation backend\n");
        return 0;
    }
    return -1;
}

void hal_close(void) {
    if (use_sim) hal_sim_close();
    else hal_revpi_close();
}

uint8_t hal_read_byte(uint16_t offset) {
    return use_sim ? hal_sim_read_byte(offset)
                   : hal_revpi_read_byte(offset);
}

void hal_write_byte(uint16_t offset, uint8_t value) {
    if (use_sim) hal_sim_write_byte(offset, value);
    else hal_revpi_write_byte(offset, value);
}

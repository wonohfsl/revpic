#include "hal.h"
#include <string.h>
#include <stdio.h>

#define SIM_IMAGE_SIZE 4096

static uint8_t sim_image[SIM_IMAGE_SIZE];
int hal_sim_init(void) {
    memset(sim_image, 0, sizeof(sim_image));
    return 0;
}

void hal_sim_close(void) {}

uint8_t hal_sim_read_byte(uint16_t offset) {
    if (offset >= SIM_IMAGE_SIZE) return 0;
    return sim_image[offset];
}

void hal_sim_write_byte(uint16_t offset, uint8_t value) {
    if (offset < SIM_IMAGE_SIZE) sim_image[offset] = value;
}

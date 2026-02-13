#include <stdio.h>
#include <unistd.h>
#include "revpi.h"

int main(void)
{
    revpi_t rpi = revpi_open("/dev/piControl0");

    int value;

    printf("=== OBJECT-STYLE MIO TEST ===\n");

    /* Test DI */
    rpi.dio.get(&rpi, 1, &value);
    printf("DI1 = %d\n", value);

    /* Test DO */
    rpi.dio.set(&rpi, 1, 1);
    rpi.dio.get(&rpi, 1, &value);
    printf("DO1 = %d\n", value);

    /* Test AI */
    rpi.ai.read(&rpi, 1, &value);
    printf("AI1 = %d\n", value);

    /* Test AO */
    rpi.ao.write(&rpi, 1, 5000);
    usleep(5000);
    rpi.ai.read(&rpi, 1, &value);
    printf("AI1 after AO1=5000: %d\n", value);

    rpi.close(&rpi);
    return 0;
}

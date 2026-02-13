#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>

#include "piControl.h"
#include "piControlIf.h"

static int read_ai(const char *name)
{
    SPIVariable var;
    uint16_t value = 0;

    snprintf(var.strVarName, sizeof(var.strVarName), "%s", name);

    if (piControlGetVariableInfo(&var) < 0)
        return -1;

    if (piControlRead(var.i16uAddress, 2, (uint8_t *)&value) < 0)
        return -1;

    return value;
}

static int write_ao(const char *name, uint16_t value)
{
    SPIVariable var;

    snprintf(var.strVarName, sizeof(var.strVarName), "%s", name);

    if (piControlGetVariableInfo(&var) < 0)
        return -1;

    if (piControlWrite(var.i16uAddress, 2, (uint8_t *)&value) < 0)
        return -1;

    return 0;
}

int main(void)
{
    if (piControlOpen() < 0) {
        printf("Cannot open piControl\n");
        return 1;
    }

    printf("=== AI1 <-> AO1 TEST ===\n");

    int ai;

    ai = read_ai("AnalogInput_1");
    printf("AI1 = %d\n", ai);

    write_ao("AnalogOutput_1", 5000);
	usleep(5000);   // 5 ms
    ai = read_ai("AnalogInput_1");
    printf("AI1 = %d\n", ai);

    write_ao("AnalogOutput_1", 10000);
	usleep(5000);   // 5 ms
    ai = read_ai("AnalogInput_1");
    printf("AI1 = %d\n", ai);

    write_ao("AnalogOutput_1", 0);
	usleep(5000);   // 5 ms
    ai = read_ai("AnalogInput_1");
    printf("AI1 = %d\n", ai);

    piControlClose();
    return 0;
}

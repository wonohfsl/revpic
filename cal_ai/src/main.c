#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "piControl.h"
#include "piControlIf.h"
#include "mio_addr.h"     // <-- use your static offsets

static int read_ai(uint16_t offset)
{
    uint16_t value = 0;

    if (piControlRead(offset, 2, (uint8_t *)&value) < 0) {
        printf("Failed to read AI at offset %u\n", offset);
        return -1;
    }

    return value;
}

static int write_ao(uint16_t offset, uint16_t value)
{
    if (piControlWrite(offset, 2, (uint8_t *)&value) < 0) {
        printf("Failed to write AO at offset %u\n", offset);
        return -1;
    }

    return 0;
}

int main(void)
{
    if (piControlOpen() < 0) {
        printf("Cannot open piControl\n");
        return 1;
    }

	int t_settle=100000;	// settling time of analog output: 100ms
	int t_sample=1000;		// sampling time of analgo input: 1ms
	
    printf("Measure after %dus. Every %dus\n", t_settle, t_sample);
	printf("REFERENCE (ao), Measured (ai), Diff(ao-ai)\n");

    int ai, ao;

	for(int j=0; j<=100; j++)
	{
		ao=10*j;
		write_ao(AO1_OFFSET, ao);
		usleep(t_settle);
		for(int i=0; i<3; i++)
		{
			ai = read_ai(AI1_OFFSET);
			printf("%d, %d, %d\n", ao, ai, ao-ai);
			usleep(t_sample);
		}
		printf("\n");
	}

    piControlClose();
    return 0;
}

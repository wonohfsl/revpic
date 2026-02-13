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

    printf("Measure after 30ms. Every 10ms\n");
	printf("input, output, diff(input-output)\n");
	
    int ai, ao;

    /* 2. AO1 = 5000 */
	ao=5000;
    write_ao(AO1_OFFSET, ao);
	usleep(30000);
	for(int i=0; i<30; i++)
	{
		ai = read_ai(AI1_OFFSET);
		printf("%d, %d, %d\n", ai, ao, ai-ao);
		usleep(10000);
	}
	printf("\n");
	
    /* 3. AO1 = 10000 */
	ao=10000;
    write_ao(AO1_OFFSET, ao);
	usleep(30000);
	for(int i=0; i<30; i++)
	{
		ai = read_ai(AI1_OFFSET);
		printf("%d, %d, %d\n", ai, ao, ai-ao);
		usleep(10000);
	}
	printf("\n");

    /* 4. AO1 = 0 */
	ao=0;
    write_ao(AO1_OFFSET, ao);
	usleep(30000);
	for(int i=0; i<30; i++)
	{
		ai = read_ai(AI1_OFFSET);
		printf("%d, %d, %d\n", ai, ao, ai-ao);
		usleep(10000);
	}
	printf("\n");

    piControlClose();
    return 0;
}

#include <stdio.h>
#include <unistd.h>
#include "mio.h"

int main(void)
{
    if (mio_init() < 0) {
        printf("Failed to init MIO\n");
        return 1;
    }

    printf("=== MIO HAL TEST ===\n");
	
    /* Test DI/DO */

    printf("DI1 = %d\n\n", mio_get_di(1));

	printf("DO3 <-- 1\n",);
    mio_set_do(3, 1);
	usleep(50000);
    printf("DO3 = %d\n", mio_get_do(3));
    printf("DI1 = %d\n\n", mio_get_di(1));

	printf("DO3 <-- 0\n");
    mio_set_do(3, 0);
	usleep(50000);
    printf("DO3 = %d\n", mio_get_do(3));
    printf("DI1 = %d\n\n", mio_get_di(1));

    /* Test AI/AO */

    printf("AI1 = %d\n\n", mio_get_ai(1));

    printf("AO1 <-- 5000\n");
    mio_set_ao(1, 5000);
    usleep(50000);
    printf("AO1 = %d\n", mio_get_ao(1));
    printf("AI1 = %d\n\n", mio_get_ai(1));

    printf("AO1 <-- 10000\n");
    mio_set_ao(1, 10000);
    usleep(50000);
    printf("AO1 = %d\n", mio_get_ao(1));
    printf("AI1 = %d\n\n", mio_get_ai(1));
	
    printf("AO1 <-- 0\n");
    mio_set_ao(1, 0);
    usleep(50000);
    printf("AO1 = %d\n", mio_get_ao(1));
    printf("AI1 = %d\n\n", mio_get_ai(1));

    return 0;
}

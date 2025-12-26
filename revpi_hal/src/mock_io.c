#include "hal.h"
#include <stdio.h>
#include <string.h>

void run_mock_console(void) {
    char line[128];

    printf("Mock I/O console. Commands:\n");
    printf("  r <offset>          - read byte\n");
    printf("  w <offset> <value>  - write byte\n");
    printf("  q                   - quit\n");

    while (1) {
        printf("> ");
        if (!fgets(line, sizeof(line), stdin))
            break;
        if (line[0] == 'q')
            break;
        if (line[0] == 'r') {
            uint16_t offset;
            if (sscanf(line + 1, "%hu", &offset) == 1) {
                uint8_t v = hal_read_byte(offset);
                printf("offset %u = %u (0x%02X)\n", offset, v, v);
            }
        } else if (line[0] == 'w') {
            uint16_t offset;
            unsigned int value;
            if (sscanf(line + 1, "%hu %u", &offset, &value) == 2) {
                hal_write_byte(offset, (uint8_t)value);
                printf("offset %u <- %u (0x%02X)\n", offset, value, value);
            }
        }
    }
}

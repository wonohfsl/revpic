#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h> // for malloc() and free()
#include "piControl.h"

#define MAX_DEVICES   64
#define MAX_VARIABLES 1024

typedef struct {
    char name[64];
} VariableEntry;

/* ---------------------------------------------------------
 * 1. Read device info using KB_GET_DEVICE_INFO_LIST
 * --------------------------------------------------------- */
int get_device_info(int fd, SDeviceInfo* devs)
{
    memset(devs, 0, sizeof(SDeviceInfo) * MAX_DEVICES);

    if (ioctl(fd, KB_GET_DEVICE_INFO_LIST, devs) < 0) {
        perror("KB_GET_DEVICE_INFO_LIST");
        return -1;
    }

    printf("=== Devices ===\n");
    for (int i = 0; i < MAX_DEVICES; i++) {
        if (!devs[i].i8uActive)
            continue;

        printf("Device %d:\n", i);
        printf("  Address:      %u\n", devs[i].i8uAddress);
        printf("  Module Type:  %u\n", devs[i].i16uModuleType);
        printf("  Input Offset: %u  Length: %u\n",
               devs[i].i16uInputOffset, devs[i].i16uInputLength);
        printf("  Output Offset:%u  Length: %u\n",
               devs[i].i16uOutputOffset, devs[i].i16uOutputLength);
        printf("\n");
    }

    return 0;
}

/* ---------------------------------------------------------
 * 2. Parse config.rsc and extract variable names
 *    from Devices[*].inp, Devices[*].out, Devices[*].mem
 * --------------------------------------------------------- */
int read_variables(VariableEntry* vars)
{
    FILE* f = fopen("./config.rsc", "r");
    if (!f) {
        printf("Please copy '/etc/revpi/config.rsc' here.\n");
        return 0;
    }

    printf("=== Reading variable names from ./config.rsc ===\n");

    /* Load entire file into memory */
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char* buf = malloc(size + 1);
    fread(buf, 1, size, f);
    buf[size] = '\0';
    fclose(f);

    int count = 0;

    /* Sections to search */
    const char* sections[] = { "\"inp\"", "\"out\"", "\"mem\"" };

    for (int s = 0; s < 3; s++) {

        char* sec = strstr(buf, sections[s]);
        if (!sec)
            continue;

        /* Find opening brace after section name */
        char* p = strchr(sec, '{');
        if (!p)
            continue;

        /* Parse until matching '}' */
        int brace = 1;
        p++;

        while (*p && brace > 0) {

            if (*p == '{') brace++;
            if (*p == '}') brace--;

            if (*p == '[') {
                /* Extract first quoted string inside [ ... ] */
                char* q1 = strchr(p, '\"');
                if (!q1) break;

                char* q2 = strchr(q1 + 1, '\"');
                if (!q2) break;

                size_t len = q2 - (q1 + 1);
                if (len > 0 && len < sizeof(vars[count].name)) {

                    strncpy(vars[count].name, q1 + 1, len);
                    vars[count].name[len] = '\0';

                    /* Skip invalid names like "," */
                    if (vars[count].name[0] != ',' &&
                        vars[count].name[0] != '\0') {

                        printf("  Found variable: %s\n", vars[count].name);
                        count++;
                    }
                }

                p = q2;
            }

            p++;
        }
    }

    free(buf);

    printf("Total variables found: %d\n\n", count);
    return count;
}

/* ---------------------------------------------------------
 * 3. For each variable:
 *    - KB_FIND_VARIABLE → offset, bit, length
 *    - KB_GET_VALUE     → actual value
 * --------------------------------------------------------- */
void get_variable_info(int fd, VariableEntry* vars, int count)
{
    printf("=== Variable Info ===\n\n");

    for (int i = 0; i < count; i++) {

        SPIVariable var;
        memset(&var, 0, sizeof(var));
        snprintf(var.strVarName, sizeof(var.strVarName), "%s", vars[i].name);

        if (ioctl(fd, KB_FIND_VARIABLE, &var) < 0) {
            printf("%s: not found by KB_FIND_VARIABLE\n\n", vars[i].name);
            continue;
        }

        printf("%s\n", vars[i].name);
        printf("  Offset: %u\n", var.i16uAddress);
        printf("  Bit:    %u\n", var.i8uBit);
        printf("  Length: %u bits\n", var.i16uLength);

        SPIValue v;
        memset(&v, 0, sizeof(v));
        v.i16uAddress = var.i16uAddress;
        v.i8uBit      = var.i8uBit;

        ioctl(fd, KB_GET_VALUE, &v);

        if (var.i16uLength == 1) {
            printf("  Value: %u (digital)\n\n", v.i8uValue);
        }
        else if (var.i16uLength == 16) {
            SPIValue high;
            memset(&high, 0, sizeof(high));
            high.i16uAddress = var.i16uAddress + 1;
            high.i8uBit      = 0;
            ioctl(fd, KB_GET_VALUE, &high);

            unsigned int full = v.i8uValue | (high.i8uValue << 8);
            printf("  Value: %u (16-bit)\n\n", full);
        }
        else {
            printf("  Value: %u (raw byte)\n\n", v.i8uValue);
        }
    }
}

int main(void)
{
    int fd = open(PICONTROL_DEVICE, O_RDWR);
    if (fd < 0) {
        perror("open " PICONTROL_DEVICE);
        return 1;
    }

    SDeviceInfo devs[MAX_DEVICES];
    VariableEntry vars[MAX_VARIABLES];

    /* Step 1: Discover devices */
    get_device_info(fd, devs);

    /* Step 2: Read variable names from config.rsc */
    int var_count = read_variables(vars);
    if (var_count <= 0) {
        fprintf(stderr, "No variables found in ./config.rsc\n");
        close(fd);
        return 1;
    }

    /* Step 3: Read variable info and values */
    get_variable_info(fd, vars, var_count);

    close(fd);
    return 0;
}

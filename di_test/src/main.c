#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include "piControl.h"

static void print_ioctl_result(const char *label, int ret)
{
    if (ret < 0) {
        printf("%s: ERROR ret=%d errno=%d (%s)\n",
               label, ret, errno, strerror(errno));
    } 
	//else {
    //    printf("%s: OK ret=%d\n", label, ret);
    //}
}

int main(void)
{
    int fd;
    SPIVariable var;
    SPIValue val;

    printf("Opening /dev/piControl0...\n");
    fd = open("/dev/piControl0", O_RDWR);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    /* ============================================================
       1. FIND DigitalInput_1
       ============================================================ */
    memset(&var, 0, sizeof(var));
    strncpy(var.strVarName, "DigitalInput_1", sizeof(var.strVarName)-1);

    int ret = ioctl(fd, KB_FIND_VARIABLE, &var);
    print_ioctl_result("KB_FIND_VARIABLE(DI1)", ret);

    printf("DI1 offset=%d bit=%d len=%d\n",
           var.i16uAddress, var.i8uBit, var.i16uLength);

    int di_offset = var.i16uAddress;
    int di_bit    = var.i8uBit;

    /* ============================================================
       2. FIND DigitalOutput_3
       ============================================================ */
    memset(&var, 0, sizeof(var));
    strncpy(var.strVarName, "DigitalOutput_3", sizeof(var.strVarName)-1);

    ret = ioctl(fd, KB_FIND_VARIABLE, &var);
    print_ioctl_result("KB_FIND_VARIABLE(DO3)", ret);

    printf("DO3 offset=%d bit=%d len=%d\n\n",
           var.i16uAddress, var.i8uBit, var.i16uLength);

    int do_offset = var.i16uAddress;
    int do_bit    = var.i8uBit;

    /* ============================================================
       3. READ DI1 (initial)
       ============================================================ */
    memset(&val, 0, sizeof(val));
    val.i16uAddress = di_offset;
    val.i8uBit      = di_bit;

    ret = ioctl(fd, KB_GET_VALUE, &val);
    print_ioctl_result("KB_GET_VALUE(DI1)", ret);
    printf("DI1 initial value: %d\n", val.i8uValue);

    /* ============================================================
       4. WRITE DO3 = 1
       ============================================================ */
    memset(&val, 0, sizeof(val));
    val.i16uAddress = do_offset;
    val.i8uBit      = do_bit;
    val.i8uValue    = 1;

    ret = ioctl(fd, KB_SET_VALUE, &val);
    print_ioctl_result("KB_SET_VALUE(DO3=1)", ret);
	printf("DO3 Set to 1\n");

    usleep(50000);  // 50 ms delay

    /* ============================================================
       5. READ DI1 after DO3=1
       ============================================================ */
    memset(&val, 0, sizeof(val));
    val.i16uAddress = di_offset;
    val.i8uBit      = di_bit;

    ret = ioctl(fd, KB_GET_VALUE, &val);
    print_ioctl_result("KB_GET_VALUE(DI1)", ret);
    printf("DI1 after Set DO3 to 1: %d\n", val.i8uValue);

    /* ============================================================
       6. WRITE DO3 = 0
       ============================================================ */
    memset(&val, 0, sizeof(val));
    val.i16uAddress = do_offset;
    val.i8uBit      = do_bit;
    val.i8uValue    = 0;

    ret = ioctl(fd, KB_SET_VALUE, &val);
    print_ioctl_result("KB_SET_VALUE(DO3=0)", ret);
	printf("DO3 Reset to 0\n");

    usleep(50000);  // 50 ms delay

    /* ============================================================
       7. READ DI1 after DO3=0
       ============================================================ */
    memset(&val, 0, sizeof(val));
    val.i16uAddress = di_offset;
    val.i8uBit      = di_bit;

    ret = ioctl(fd, KB_GET_VALUE, &val);
    print_ioctl_result("KB_GET_VALUE(DI1)", ret);
    printf("DI1 after Reset DO3 to 0: %d\n", val.i8uValue);

    close(fd);
    return 0;
}
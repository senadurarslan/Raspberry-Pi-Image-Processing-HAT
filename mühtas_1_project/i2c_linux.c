#include "i2c_linux.h"
#include <linux/i2c-dev.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdio.h>

int i2c_open(const char *device, uint8_t addr)
{
    int fd = open(device, O_RDWR);
    if (fd < 0) {
        perror("open i2c");
        return -1;
    }
    if (ioctl(fd, I2C_SLAVE, addr) < 0) {
        perror("I2C_SLAVE");
        close(fd);
        return -1;
    }
    return fd;
}

void i2c_close(int fd)
{
    if (fd >= 0) close(fd);
}

int i2c_write_reg8(int fd, uint8_t reg, uint8_t val)
{
    uint8_t buf[2] = { reg, val };
    if (write(fd, buf, 2) != 2) {
        perror("i2c write");
        return -1;
    }
    return 0;
}

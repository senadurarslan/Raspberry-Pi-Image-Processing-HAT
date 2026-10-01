#ifndef I2C_LINUX_H
#define I2C_LINUX_H

#include <stdint.h>

int i2c_open(const char *device, uint8_t addr);
void i2c_close(int fd);
int i2c_write_reg8(int fd, uint8_t reg, uint8_t val);

#endif

#ifndef SPI_LINUX_H
#define SPI_LINUX_H

#include <stddef.h>
#include <stdint.h>

int spi_open(const char *device, uint32_t speed_hz);
void spi_close(int fd);
int spi_transfer(int fd, const uint8_t *tx, uint8_t *rx, size_t len);

#endif

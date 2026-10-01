#include "spi_linux.h"
#include <linux/spi/spidev.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

int spi_open(const char *device, uint32_t speed_hz)
{
    int fd = open(device, O_RDWR);
    if (fd < 0) {
        perror("open spi");
        return -1;
    }

    uint8_t mode = 0;   // CPOL=0, CPHA=0 (ArduCAM sabit) :contentReference[oaicite:3]{index=3}
    uint8_t bits = 8;

    if (ioctl(fd, SPI_IOC_WR_MODE, &mode) < 0) {
        perror("SPI_IOC_WR_MODE");
        goto err;
    }
    if (ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits) < 0) {
        perror("SPI_IOC_WR_BITS_PER_WORD");
        goto err;
    }
    if (ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed_hz) < 0) {
        perror("SPI_IOC_WR_MAX_SPEED_HZ");
        goto err;
    }

    return fd;
err:
    close(fd);
    return -1;
}

void spi_close(int fd)
{
    if (fd >= 0) close(fd);
}

int spi_transfer(int fd, const uint8_t *tx, uint8_t *rx, size_t len)
{
    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)tx,
        .rx_buf = (unsigned long)rx,
        .len = len,
        .speed_hz = 0,   // default
        .delay_usecs = 0,
        .bits_per_word = 0,
    };

    int ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
    if (ret < 1) {
        perror("SPI_IOC_MESSAGE");
        return -1;
    }
    return 0;
}

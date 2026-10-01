// test_hal.c
#include <stdio.h>
#include <stdint.h>
#include "spi_linux.h"
#include "i2c_linux.h"

int main(void)
{
    // SPI kamera → /dev/spidev0.0 (8MHz)
    int spi_cam = spi_open("/dev/spidev0.0", 8 * 1000 * 1000);
    if (spi_cam < 0) {
        printf("SPI camera open FAILED\n");
        return 1;
    } else {
        printf("SPI camera open OK (/dev/spidev0.0)\n");
    }

    // SPI TFT → /dev/spidev0.1 (şimdilik sadece aç-kapa)
    int spi_tft = spi_open("/dev/spidev0.1", 8 * 1000 * 1000);
    if (spi_tft < 0) {
        printf("SPI TFT open FAILED\n");
    } else {
        printf("SPI TFT open OK (/dev/spidev0.1)\n");
    }

    // I2C kamera sensörü → /dev/i2c-1, addr = 0x30
    int i2c_cam = i2c_open("/dev/i2c-1", 0x30);
    if (i2c_cam < 0) {
        printf("I2C camera open FAILED\n");
    } else {
        printf("I2C camera open OK (/dev/i2c-1, addr=0x30)\n");
    }

    // Basit bir SPI round-trip testi (kamera SPI hattında)
    uint8_t tx[1] = { 0x00 };
    uint8_t rx[1] = { 0x00 };
    if (spi_transfer(spi_cam, tx, rx, 1) == 0) {
        printf("SPI camera transfer OK (rx=0x%02X)\n", rx[0]);
    } else {
        printf("SPI camera transfer FAILED\n");
    }

    // Burada gerçek bir I2C register testi OV2640/ArduChip seviyesinde olacak,
    // ama henüz o katmanı yazmadık. Şimdilik sadece open/close test ettin.

    if (i2c_cam >= 0) i2c_close(i2c_cam);
    if (spi_tft >= 0) spi_close(spi_tft);
    if (spi_cam >= 0) spi_close(spi_cam);

    return 0;
}

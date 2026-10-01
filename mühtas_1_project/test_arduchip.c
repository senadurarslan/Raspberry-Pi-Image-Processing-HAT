// test_arduchip.c
#include <stdio.h>
#include <stdint.h>

#include "spi_linux.h"
#include "arduchip.h"

int main(void)
{
    // Kamera SPI: /dev/spidev0.0
    int spi_cam = spi_open("/dev/spidev0.0", 8 * 1000 * 1000);
    if (spi_cam < 0) {
        printf("SPI camera open FAILED\n");
        return 1;
    }

    struct arduchip cam;
    arduchip_init(&cam, spi_cam);

    // 1) TEST1 register testi (0x00)
    printf("Writing 0x55 to ARDUCHIP_TEST1 (0x00)...\n");
    arduchip_write_reg(&cam, ARDUCHIP_TEST1, 0x55);
    uint8_t test_val = arduchip_read_reg(&cam, ARDUCHIP_TEST1);
    printf("Read back: 0x%02X\n", test_val);

    if (test_val != 0x55) {
        printf("ERROR: SPI interface / ArduChip TEST1 mismatch!\n");
        spi_close(spi_cam);
        return 1;
    } else {
        printf("OK: SPI interface / ArduChip TEST1 PASSED.\n");
    }

    // 2) Versiyon register’ı oku
    uint8_t ver = arduchip_read_reg(&cam, ARDUCHIP_VER);
    printf("ArduChip version register (0x40) = 0x%02X\n", ver);
    // Mini 2MP için genelde 0x40 döner, ama rev’e göre değişebilir.

    // 3) FIFO length’i oku (sadece test, capture yapmadık)
    uint32_t len = arduchip_read_fifo_length(&cam);
    printf("FIFO length (no capture yet) = %u bytes\n", len);

    spi_close(spi_cam);
    return 0;
}

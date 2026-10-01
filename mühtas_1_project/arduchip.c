// arduchip.c
#include "arduchip.h"
#include "spi_linux.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

void arduchip_init(struct arduchip *chip, int spi_fd)
{
    chip->spi_fd = spi_fd;
}

// ArduChip SPI protokolü:
// bit7 = 0 → READ, bit7 = 1 → WRITE
// bit6:0 = register address.
uint8_t arduchip_read_reg(struct arduchip *chip, uint8_t addr)
{
    uint8_t tx[2];
    uint8_t rx[2];

    tx[0] = addr & 0x7F;  // read
    tx[1] = 0x00;

    if (spi_transfer(chip->spi_fd, tx, rx, 2) < 0) {
        fprintf(stderr, "arduchip_read_reg: spi error on addr 0x%02X\n", addr);
        return 0;
    }

    return rx[1];
}

void arduchip_write_reg(struct arduchip *chip, uint8_t addr, uint8_t val)
{
    uint8_t tx[2];
    uint8_t rx[2];

    tx[0] = addr | 0x80; // write
    tx[1] = val;

    if (spi_transfer(chip->spi_fd, tx, rx, 2) < 0) {
        fprintf(stderr, "arduchip_write_reg: spi error on addr 0x%02X\n", addr);
    }
}

// FIFO pointer reset + flag clear
void arduchip_fifo_reset(struct arduchip *chip)
{
    // bit4 = reset write pointer
    // bit5 = reset read pointer
    // bit0 = clear write-done flag
    uint8_t val = (1 << 4) | (1 << 5) | (1 << 0);
    arduchip_write_reg(chip, ARDUCHIP_FIFO, val);
}

// flush, capture öncesi gibi düşünebilirsin (pointer + flag)
void arduchip_flush_fifo(struct arduchip *chip)
{
    arduchip_fifo_reset(chip);
}

// capture başlatır (bit1)
void arduchip_start_capture(struct arduchip *chip)
{
    arduchip_write_reg(chip, ARDUCHIP_FIFO, (1 << 1));
}

// capture bitene kadar bekler, timeout_ms > 0 ise zaman aşımı kontrolü yapar
int arduchip_wait_capture_done(struct arduchip *chip, uint32_t timeout_ms)
{
    struct timespec start, now;
    if (timeout_ms > 0) {
        clock_gettime(CLOCK_MONOTONIC, &start);
    }

    for (;;) {
        uint8_t v = arduchip_read_reg(chip, ARDUCHIP_TRIG);
        if (v & ARDUCHIP_CAP_DONE_MASK) {
            return 0; // OK
        }

        if (timeout_ms > 0) {
            clock_gettime(CLOCK_MONOTONIC, &now);
            uint64_t elapsed_ms =
                (uint64_t)(now.tv_sec - start.tv_sec) * 1000ULL +
                (uint64_t)(now.tv_nsec - start.tv_nsec) / 1000000ULL;
            if (elapsed_ms > timeout_ms) {
                return -1; // timeout
            }
        }

        // sensörü boğmamak için küçük uyku
        usleep(1000); // 1ms
    }
}

// FIFO length registerlarını okur (18-bit)
uint32_t arduchip_read_fifo_length(struct arduchip *chip)
{
    uint8_t len1 = arduchip_read_reg(chip, ARDUCHIP_FIFO_SIZE1);
    uint8_t len2 = arduchip_read_reg(chip, ARDUCHIP_FIFO_SIZE2);
    uint8_t len3 = arduchip_read_reg(chip, ARDUCHIP_FIFO_SIZE3) & 0x07;

    uint32_t len = ((uint32_t)len3 << 16) | ((uint32_t)len2 << 8) | len1;
    return len;
}

// FIFO'dan len kadar JPEG byte'ı burst modda okur.
// IMPORTANT:
//   - Tek transfer içinde 0x3C komutu + len byte clock ediyoruz.
//   - rx[0] dummy, rx[1..len] gerçek JPEG byte'larıdır.
int arduchip_read_fifo_burst(struct arduchip *chip, uint8_t *buf, uint32_t len)
{
    const uint32_t CHUNK = 1024; // kernel sınırına takılmamak için küçük tut
    uint8_t *tx = malloc(CHUNK + 1);
    uint8_t *rx = malloc(CHUNK + 1);

    if (!tx || !rx) {
        fprintf(stderr, "burst: malloc failed\n");
        free(tx); free(rx);
        return -1;
    }

    // Komut dışındaki byte'lar dummy (0x00)
    memset(tx, 0x00, CHUNK + 1);

    uint32_t offset = 0;

    while (offset < len) {
        uint32_t this = len - offset;
        if (this > CHUNK) this = CHUNK;

        tx[0] = ARDUCHIP_BURST_FIFO_READ;   // 0x3C

        // 1 byte komut + this byte data clock ediyoruz
        if (spi_transfer(chip->spi_fd, tx, rx, this + 1) < 0) {
            perror("burst chunk spi_transfer");
            free(tx); free(rx);
            return -1;
        }

        // rx[0] = komut cevabı (çöpe), rx[1..this] = FIFO verisi
        memcpy(buf + offset, rx + 1, this);
        offset += this;
    }

    free(tx);
    free(rx);
    return 0;
}


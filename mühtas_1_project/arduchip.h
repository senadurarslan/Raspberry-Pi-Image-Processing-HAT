// arduchip.h
#ifndef ARDUCHIP_H
#define ARDUCHIP_H

#include <stdint.h>
#include <stddef.h>

// ArduCAM / ArduChip register adresleri
// Software & Hardware Application Note'tan alındı.
#define ARDUCHIP_TEST1      0x00
#define ARDUCHIP_FIFO       0x04
#define ARDUCHIP_TRIG       0x41
#define ARDUCHIP_FIFO_SIZE1 0x42
#define ARDUCHIP_FIFO_SIZE2 0x43
#define ARDUCHIP_FIFO_SIZE3 0x44
#define ARDUCHIP_VER        0x40

// CAP_DONE için trig register bit3 kullanılıyor.
#define ARDUCHIP_CAP_DONE_MASK 0x08

// Burst FIFO okuma komutu (adres)
#define ARDUCHIP_BURST_FIFO_READ 0x3C

struct arduchip {
    int spi_fd;   // /dev/spidev0.0 fd
};

// Basit init (sadece fd’yi kaydediyoruz)
void arduchip_init(struct arduchip *chip, int spi_fd);

// Register erişimi
uint8_t arduchip_read_reg(struct arduchip *chip, uint8_t addr);
void    arduchip_write_reg(struct arduchip *chip, uint8_t addr, uint8_t val);

// FIFO / capture kontrolü
void     arduchip_fifo_reset(struct arduchip *chip);
void     arduchip_flush_fifo(struct arduchip *chip);
void     arduchip_start_capture(struct arduchip *chip);
// timeout_ms = 0 ise sonsuz bekler, aksi halde ms cinsinden timeout
int      arduchip_wait_capture_done(struct arduchip *chip, uint32_t timeout_ms);
uint32_t arduchip_read_fifo_length(struct arduchip *chip);

// FIFO burst read: len kadar JPEG byte’ını buf’a yazar.
// 0 → OK, <0 → hata.
int      arduchip_read_fifo_burst(struct arduchip *chip, uint8_t *buf, uint32_t len);

#endif

// st7735.c
#include "st7735.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>

// =======================
//  GPIO (sysfs) helper'lar
// =======================
static int gpio_export(int gpio)
{
    char buf[64];
    int fd = open("/sys/class/gpio/export", O_WRONLY);
    if (fd < 0) {
        perror("gpio_export: open");
        return -1;
    }
    int len = snprintf(buf, sizeof(buf), "%d", gpio);
    if (write(fd, buf, len) != len) {
        perror("gpio_export: write");
        close(fd);
        return -1;
    }
    close(fd);
    return 0;
}

static int gpio_unexport(int gpio)
{
    char buf[64];
    int fd = open("/sys/class/gpio/unexport", O_WRONLY);
    if (fd < 0) {
        // Sessizce geçebiliriz
        return -1;
    }
    int len = snprintf(buf, sizeof(buf), "%d", gpio);
    write(fd, buf, len);
    close(fd);
    return 0;
}

static int gpio_set_dir(int gpio, int is_output)
{
    char path[64];
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/direction", gpio);
    int fd = open(path, O_WRONLY);
    if (fd < 0) {
        perror("gpio_set_dir: open");
        return -1;
    }
    if (is_output)
        write(fd, "out", 3);
    else
        write(fd, "in", 2);
    close(fd);
    return 0;
}

static int gpio_write(int gpio, int value)
{
    char path[64];
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/value", gpio);
    int fd = open(path, O_WRONLY);
    if (fd < 0) {
        perror("gpio_write: open");
        return -1;
    }
    if (value)
        write(fd, "1", 1);
    else
        write(fd, "0", 1);
    close(fd);
    return 0;
}

// =======================
//  SPI helper
// =======================
static int spi_open_dev(const char *dev, uint32_t speed_hz)
{
    int fd = open(dev, O_RDWR);
    if (fd < 0) {
        perror("spi_open_dev: open");
        return -1;
    }

    uint8_t mode = SPI_MODE_0;
    uint8_t bits = 8;

    if (ioctl(fd, SPI_IOC_WR_MODE, &mode) < 0) {
        perror("SPI_IOC_WR_MODE");
        close(fd);
        return -1;
    }

    if (ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits) < 0) {
        perror("SPI_IOC_WR_BITS_PER_WORD");
        close(fd);
        return -1;
    }

    if (ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed_hz) < 0) {
        perror("SPI_IOC_WR_MAX_SPEED_HZ");
        close(fd);
        return -1;
    }

    return fd;
}

static int spi_write_bytes(int fd, const uint8_t *data, size_t len)
{
    struct spi_ioc_transfer tr;
    memset(&tr, 0, sizeof(tr));
    tr.tx_buf = (unsigned long)data;
    tr.len    = len;

    if (ioctl(fd, SPI_IOC_MESSAGE(1), &tr) < 0) {
        perror("SPI_IOC_MESSAGE");
        return -1;
    }
    return 0;
}

// =======================
//  Komut / Veri helper'ları
// =======================
static void st7735_write_command(struct st7735 *lcd, uint8_t cmd)
{
    gpio_write(lcd->dc_gpio, 0); // Command
    spi_write_bytes(lcd->spi_fd, &cmd, 1);
}

static void st7735_write_data(struct st7735 *lcd, const uint8_t *data, size_t len)
{
    gpio_write(lcd->dc_gpio, 1); // Data
    spi_write_bytes(lcd->spi_fd, data, len);
}

static void st7735_write_data8(struct st7735 *lcd, uint8_t d)
{
    gpio_write(lcd->dc_gpio, 1);
    spi_write_bytes(lcd->spi_fd, &d, 1);
}

// =======================
//  Donanımsal reset
// =======================
static void st7735_hw_reset(struct st7735 *lcd)
{
    if (lcd->rst_gpio >= 0) {
        gpio_write(lcd->rst_gpio, 1);
        usleep(1000);
        gpio_write(lcd->rst_gpio, 0);
        usleep(10000);
        gpio_write(lcd->rst_gpio, 1);
        usleep(100000);
    } else {
        // Reset pini yoksa biraz bekle
        usleep(150000);
    }
}

// =======================
//  ST7735 Init Sequence (128x160 1.8" klasik)
// =======================
static void st7735_init_sequence(struct st7735 *lcd)
{
    // Sleep Out
    st7735_write_command(lcd, 0x11);
    usleep(120000);

    // Frame rate control - normal mode
    st7735_write_command(lcd, 0xB1);
    uint8_t frctrl1[] = {0x01, 0x2C, 0x2D};
    st7735_write_data(lcd, frctrl1, sizeof(frctrl1));

    // Frame rate control - idle mode
    st7735_write_command(lcd, 0xB2);
    uint8_t frctrl2[] = {0x01, 0x2C, 0x2D};
    st7735_write_data(lcd, frctrl2, sizeof(frctrl2));

    // Frame rate control - partial mode
    st7735_write_command(lcd, 0xB3);
    uint8_t frctrl3[] = {0x01, 0x2C, 0x2D, 0x01, 0x2C, 0x2D};
    st7735_write_data(lcd, frctrl3, sizeof(frctrl3));

    // Power control 1
    st7735_write_command(lcd, 0xC0);
    uint8_t pwctr1[] = {0xA2, 0x02, 0x84};
    st7735_write_data(lcd, pwctr1, sizeof(pwctr1));

    // Power control 2
    st7735_write_command(lcd, 0xC1);
    uint8_t pwctr2[] = {0xC5};
    st7735_write_data(lcd, pwctr2, sizeof(pwctr2));

    // Power control 3
    st7735_write_command(lcd, 0xC2);
    uint8_t pwctr3[] = {0x0A, 0x00};
    st7735_write_data(lcd, pwctr3, sizeof(pwctr3));

    // Power control 4
    st7735_write_command(lcd, 0xC3);
    uint8_t pwctr4[] = {0x8A, 0x2A};
    st7735_write_data(lcd, pwctr4, sizeof(pwctr4));

    // Power control 5
    st7735_write_command(lcd, 0xC4);
    uint8_t pwctr5[] = {0x8A, 0xEE};
    st7735_write_data(lcd, pwctr5, sizeof(pwctr5));

    // VCOM
    st7735_write_command(lcd, 0xC5);
    uint8_t vcom[] = {0x0E};
    st7735_write_data(lcd, vcom, sizeof(vcom));

    // Memory access control
    // 0xC0: row/col order, RGB
    st7735_write_command(lcd, 0x36);
    st7735_write_data8(lcd, 0xC0);

    // Color mode: 16-bit
    st7735_write_command(lcd, 0x3A);
    st7735_write_data8(lcd, 0x05);

    // Column address set (0..127)
    st7735_write_command(lcd, 0x2A);
    uint8_t col_addr[] = {0x00, 0x00, 0x00, 0x7F};
    st7735_write_data(lcd, col_addr, sizeof(col_addr));

    // Row address set (0..159)
    st7735_write_command(lcd, 0x2B);
    uint8_t row_addr[] = {0x00, 0x00, 0x00, 0x9F};
    st7735_write_data(lcd, row_addr, sizeof(row_addr));

    // Display ON
    st7735_write_command(lcd, 0x29);
    usleep(100000);
}

// =======================
//  Public API
// =======================
int st7735_init(struct st7735 *lcd,
                const char *spi_dev,
                int dc_gpio,
                int rst_gpio,
                uint32_t spi_speed_hz)
{
    if (!lcd) return -1;

    lcd->width  = 128;
    lcd->height = 160;
    lcd->dc_gpio  = dc_gpio;
    lcd->rst_gpio = rst_gpio;

    // GPIO export + direction
    gpio_export(dc_gpio);
    gpio_set_dir(dc_gpio, 1);

    if (rst_gpio >= 0) {
        gpio_export(rst_gpio);
        gpio_set_dir(rst_gpio, 1);
    }

    // SPI aç
    lcd->spi_fd = spi_open_dev(spi_dev, spi_speed_hz);
    if (lcd->spi_fd < 0) {
        fprintf(stderr, "st7735_init: spi_open_dev failed\n");
        return -1;
    }

    // Donanımsal reset + init
    st7735_hw_reset(lcd);
    st7735_init_sequence(lcd);

    // Başlangıçta ekranı siyah yap
    st7735_fill_screen(lcd, 0x0000);

    return 0;
}

void st7735_close(struct st7735 *lcd)
{
    if (!lcd) return;

    if (lcd->spi_fd >= 0) {
        close(lcd->spi_fd);
        lcd->spi_fd = -1;
    }
    if (lcd->dc_gpio >= 0) {
        gpio_unexport(lcd->dc_gpio);
    }
    if (lcd->rst_gpio >= 0) {
        gpio_unexport(lcd->rst_gpio);
    }
}

void st7735_set_addr_window(struct st7735 *lcd,
                            uint8_t x0, uint8_t y0,
                            uint8_t x1, uint8_t y1)
{
    // Column
    st7735_write_command(lcd, 0x2A);
    uint8_t data_col[4] = {0x00, x0, 0x00, x1};
    st7735_write_data(lcd, data_col, 4);

    // Row
    st7735_write_command(lcd, 0x2B);
    uint8_t data_row[4] = {0x00, y0, 0x00, y1};
    st7735_write_data(lcd, data_row, 4);

    // RAM write
    st7735_write_command(lcd, 0x2C);
}

void st7735_fill_screen(struct st7735 *lcd, uint16_t color)
{
    uint16_t w = lcd->width;
    uint16_t h = lcd->height;
    size_t n   = (size_t)w * h;

    st7735_set_addr_window(lcd, 0, 0, w - 1, h - 1);

    gpio_write(lcd->dc_gpio, 1);

    uint8_t buf[2];
    buf[0] = (color >> 8) & 0xFF;
    buf[1] = (color >> 0) & 0xFF;

    for (size_t i = 0; i < n; i++) {
        spi_write_bytes(lcd->spi_fd, buf, 2);
    }
}

void st7735_draw_rgb565(struct st7735 *lcd,
                        uint16_t x, uint16_t y,
                        uint16_t w, uint16_t h,
                        const uint16_t *frame)
{
    if (!frame) return;
    if ((x + w) > lcd->width)  return;
    if ((y + h) > lcd->height) return;

    st7735_set_addr_window(lcd, x, y, x + w - 1, y + h - 1);

    size_t n     = (size_t)w * (size_t)h;
    size_t bytes = n * 2;

    // Eğer renkler saçma çıkarsa, burada byte swap yaparız.
    gpio_write(lcd->dc_gpio, 1);
    spi_write_bytes(lcd->spi_fd, (const uint8_t *)frame, bytes);
}

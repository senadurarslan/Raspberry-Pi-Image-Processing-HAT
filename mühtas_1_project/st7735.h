// st7735.h
#ifndef ST7735_H
#define ST7735_H

#include <stdint.h>

// Basit ST7735 sürücü yapısı
struct st7735 {
    int spi_fd;      // /dev/spidevX.Y file descriptor
    int dc_gpio;     // Data/Command GPIO (A0 pini)
    int rst_gpio;    // Reset GPIO
    int width;       // Ekran genişliği (genelde 128)
    int height;      // Ekran yüksekliği (genelde 160)
};

// ST7735 ekranını başlat
//  spi_dev     : Örn. "/dev/spidev0.1"
//  dc_gpio     : A0 (DC) pinine bağlı BCM GPIO (sende: 24)
//  rst_gpio    : RESET pinine bağlı BCM GPIO (sende: 25)
//  spi_speed_hz: SPI hızı (örn. 8000000 = 8MHz)
int st7735_init(struct st7735 *lcd,
                const char *spi_dev,
                int dc_gpio,
                int rst_gpio,
                uint32_t spi_speed_hz);

// SPI + GPIO kaynaklarını serbest bırak
void st7735_close(struct st7735 *lcd);

// Ekranı tek renge boya (RGB565)
void st7735_fill_screen(struct st7735 *lcd, uint16_t color);

// Pencere (adres aralığı) ayarla
// x0,y0 : sol-üst
// x1,y1 : sağ-alt (dahil)
void st7735_set_addr_window(struct st7735 *lcd,
                            uint8_t x0, uint8_t y0,
                            uint8_t x1, uint8_t y1);

// Verilen RGB565 frame'i ekrana bas
// frame : w*h adet RGB565 piksel
void st7735_draw_rgb565(struct st7735 *lcd,
                        uint16_t x, uint16_t y,
                        uint16_t w, uint16_t h,
                        const uint16_t *frame);

#endif // ST7735_H

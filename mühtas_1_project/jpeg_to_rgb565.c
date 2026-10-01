#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "picojpeg.h"
#include "jpeg_to_rgb565.h"

// JPEG verisini RAM'den okuyan callback
typedef struct {
    const uint8_t *buf;
    size_t size;
    size_t offset;
} JpegIn;

static unsigned char pjpeg_memory_callback(unsigned char *pBuf,
                                           unsigned char buf_size,
                                           unsigned char *pBytes_actually_read,
                                           void *pCallback_data)
{
    JpegIn *in = (JpegIn *)pCallback_data;

    if (in->offset >= in->size) {
        *pBytes_actually_read = 0;
        return 0; // EOF
    }

    size_t remaining = in->size - in->offset;
    if (remaining > buf_size) remaining = buf_size;

    memcpy(pBuf, in->buf + in->offset, remaining);
    in->offset += remaining;
    *pBytes_actually_read = (unsigned char)remaining;

    return 0; // OK
}

// Basit RGB888 -> RGB565
static inline uint16_t rgb888_to_rgb565(uint8_t r, uint8_t g, uint8_t b)
{
    return (uint16_t)(((r & 0xF8) << 8) |
                      ((g & 0xFC) << 3) |
                      ((b >> 3)));
}

// JPEG'i decode edip RGB565 buffer oluşturan fonksiyon
int jpeg_to_rgb565(const uint8_t *jpg_buf,
                   size_t jpg_size,
                   uint16_t **out_pixels,
                   int *out_width,
                   int *out_height)
{
    pjpeg_image_info_t info;
    JpegIn in;
    int status;

    if (!jpg_buf || jpg_size == 0 || !out_pixels || !out_width || !out_height) {
        fprintf(stderr, "jpeg_to_rgb565: geçersiz parametre\n");
        return -1;
    }

    in.buf = jpg_buf;
    in.size = jpg_size;
    in.offset = 0;

    // reduce = 0 -> full resolution
    status = pjpeg_decode_init(&info,
                               pjpeg_memory_callback,
                               &in,
                               0);

    if (status) {
        fprintf(stderr, "pjpeg_decode_init failed, status = %d\n", status);
        return -2;
    }

    int width  = info.m_width;
    int height = info.m_height;

    if (width <= 0 || height <= 0) {
        fprintf(stderr, "jpeg_to_rgb565: geçersiz boyut %dx%d\n", width, height);
        return -3;
    }

    size_t num_pixels = (size_t)width * (size_t)height;
    uint16_t *pixels = (uint16_t *)malloc(num_pixels * sizeof(uint16_t));
    if (!pixels) {
        fprintf(stderr, "jpeg_to_rgb565: malloc %zu bytes FAILED\n",
                num_pixels * sizeof(uint16_t));
        return -4;
    }

    // Çıkış parametrelerini şimdilik doldur
    *out_pixels = pixels;
    *out_width  = width;
    *out_height = height;

    int mcu_x = 0;
    int mcu_y = 0;

    const int mcu_w = info.m_MCUWidth;   // MCU genişliği (piksel)
    const int mcu_h = info.m_MCUHeight;  // MCU yüksekliği (piksel)

    const int mcus_per_row = info.m_MCUSPerRow;
    const int mcus_per_col = info.m_MCUSPerCol;

    const int blocks_per_row = mcu_w / 8;
    const int blocks_per_col = mcu_h / 8;
    const int max_block_index = blocks_per_row * blocks_per_col;
    const int max_pixel_index = max_block_index * 64; // 8x8 bloklar

    // MCU MCU decode ediyoruz
    for (;;) {
        status = pjpeg_decode_mcu();

        if (status) {
            if (status == PJPG_NO_MORE_BLOCKS) {
                // bitti
                break;
            }

            fprintf(stderr, "pjpeg_decode_mcu failed, status = %d\n", status);
            free(pixels);
            *out_pixels = NULL;
            return -5;
        }

        if (mcu_y >= mcus_per_col) {
            // güvenlik
            break;
        }

        // Bu MCU içindeki her piksel
        for (int by = 0; by < mcu_h; ++by) {
            int py = mcu_y * mcu_h + by;
            if (py >= height) {
                continue; // resmin dışında
            }

            for (int bx = 0; bx < mcu_w; ++bx) {
                int px = mcu_x * mcu_w + bx;
                if (px >= width) {
                    continue; // resmin dışında
                }

                // MCU içi -> blok/piksel index hesabı
                int block_x = bx / 8;
                int block_y = by / 8;
                int local_x = bx % 8;
                int local_y = by % 8;

                int block_index = block_y * blocks_per_row + block_x;
                if (block_index < 0 || block_index >= max_block_index) {
                    continue; // güvenlik
                }

                int pixel_index = block_index * 64 + local_y * 8 + local_x;
                if (pixel_index < 0 || pixel_index >= max_pixel_index) {
                    continue; // güvenlik
                }

                uint16_t color565;

                if (info.m_scanType == PJPG_GRAYSCALE) {
                    uint8_t g = info.m_pMCUBufR[pixel_index];
                    color565 = rgb888_to_rgb565(g, g, g);
                } else {
                    uint8_t r = info.m_pMCUBufR[pixel_index];
                    uint8_t g = info.m_pMCUBufG[pixel_index];
                    uint8_t b = info.m_pMCUBufB[pixel_index];
                    color565 = rgb888_to_rgb565(r, g, b);
                }

                size_t dst_index = (size_t)py * (size_t)width + (size_t)px;
                if (dst_index < num_pixels) {
                    pixels[dst_index] = color565;
                }
            }
        }

        // Sıradaki MCU
        mcu_x++;
        if (mcu_x == mcus_per_row) {
            mcu_x = 0;
            mcu_y++;
        }
    }

    return 0;
}

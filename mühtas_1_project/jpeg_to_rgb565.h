#ifndef JPEG_TO_RGB565_H
#define JPEG_TO_RGB565_H

#include <stdint.h>
#include <stddef.h>

// Dönüş değeri:
//  0  -> OK
// <0  -> hata
//
// jpg_buf / jpg_size : JPEG verisi (RAM'de)
// out_pixels         : malloc edilmiş RGB565 buffer (width*height eleman)
// out_width/out_height: görüntü boyutları
int jpeg_to_rgb565(const uint8_t *jpg_buf,
                   size_t jpg_size,
                   uint16_t **out_pixels,
                   int *out_width,
                   int *out_height);

#endif

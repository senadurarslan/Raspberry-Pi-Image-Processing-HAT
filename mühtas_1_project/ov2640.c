// ov2640.c
#include "ov2640.h"
#include "ov2640_regs.h"
#include "i2c_linux.h"

#include <stdio.h>

// Tablo yazma helper'ı
static int ov2640_write_reg_table(int i2c_fd, const struct sensor_reg *tbl)
{
    for (int i = 0; ; i++) {
        uint8_t reg = tbl[i].reg;
        uint8_t val = tbl[i].val;

        if (reg == REG_TERM_8BIT && val == VAL_TERM_8BIT) {
            // Tablo sonu
            break;
        }

        if (i2c_write_reg8(i2c_fd, reg, val) < 0) {
            fprintf(stderr, "OV2640: reg write failed at 0x%02X = 0x%02X\n", reg, val);
            return -1;
        }
        // Gerekirse burada küçük delay'ler eklenebilir.
    }
    return 0;
}

// Sensör genel init (power-up, format, timing vs.)
int ov2640_init(int i2c_fd)
{
    // Burada ov2640_init_reg_tbl sensörü usable state'e getirir.
    if (ov2640_write_reg_table(i2c_fd, ov2640_init_reg_tbl) < 0) {
        fprintf(stderr, "OV2640: init table failed\n");
        return -1;
    }
    return 0;
}

// JPEG moduna al, 160x120 QQVGA çözünürlük seç
int ov2640_set_qqvga_jpeg(int i2c_fd)
{
    if (ov2640_write_reg_table(i2c_fd, ov2640_qqvga_jpeg_tbl) < 0) {
        fprintf(stderr, "OV2640: qqvga jpeg table failed\n");
        return -1;
    }
    return 0;
}

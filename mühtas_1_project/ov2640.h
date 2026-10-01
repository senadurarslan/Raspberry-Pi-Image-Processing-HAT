// ov2640.h
#ifndef OV2640_H
#define OV2640_H

#include <stdint.h>

// OV2640 I2C 7-bit adresi (datasheet: 0x60/0x61 8-bit → 0x30 7-bit)
#define OV2640_I2C_ADDR 0x30

// Dışarıdan sadece bu iki fonksiyona ihtiyaç duyacağız:
// 1) sensörü init etmek
// 2) JPEG modunda 160x120 (QQVGA) çözünürlük seçmek

// i2c_fd: i2c_open("/dev/i2c-1", OV2640_I2C_ADDR) ile açtığın fd
int ov2640_init(int i2c_fd);
int ov2640_set_qqvga_jpeg(int i2c_fd);

#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>

#include "spi_linux.h"
#include "i2c_linux.h"
#include "arduchip.h"
#include "ov2640.h"

#define MAX_JPEG_SIZE (400 * 1024)

int main(void)
{
    int spi_cam = spi_open("/dev/spidev0.0", 8000000);
    if (spi_cam < 0) return 1;

    struct arduchip cam;
    arduchip_init(&cam, spi_cam);

    int i2c_cam = i2c_open("/dev/i2c-1", OV2640_I2C_ADDR);
    if (i2c_cam < 0) { spi_close(spi_cam); return 1; }

    // 1) ArduChip SPI Test
    arduchip_write_reg(&cam, ARDUCHIP_TEST1, 0x55);
    if (arduchip_read_reg(&cam, ARDUCHIP_TEST1) != 0x55) {
        fprintf(stderr, "SPI Test Hatasi!\n");
        return 1;
    }

    // 2) Sensör Başlatma
    ov2640_init(i2c_cam);
    ov2640_set_qqvga_jpeg(i2c_cam);
    sleep(1); 

    // --- KRİTİK NOKTA: DUMMY CAPTURE (Gecikmeyi siler) ---
    // Önce içeride kalmış olabilecek eski kareyi tetikleyip bitiriyoruz
    arduchip_start_capture(&cam);
    arduchip_wait_capture_done(&cam, 1000);
    arduchip_write_reg(&cam, ARDUCHIP_FIFO, 0x01); // Temizle
    usleep(50000); // 50ms bekle

    // --- ASIL CAPTURE ---
    // Şimdi tertemiz bir başlangıç yapıyoruz
    arduchip_write_reg(&cam, ARDUCHIP_FIFO, 0x10); // Read Pointer Reset
    arduchip_write_reg(&cam, ARDUCHIP_FIFO, 0x20); // Write Pointer Reset
    
    printf("Gercek zamanli goruntu yakalaniyor...\n");
    arduchip_start_capture(&cam);

    if (arduchip_wait_capture_done(&cam, 2000) < 0) {
        fprintf(stderr, "Yakalama zaman asimi!\n");
        return 1;
    }

    // 3) Boyutu Oku ve Hafıza Ayır
    uint32_t jpeg_len = arduchip_read_fifo_length(&cam);
    if (jpeg_len == 0 || jpeg_len > MAX_JPEG_SIZE) {
        fprintf(stderr, "Hatali boyut: %u\n", jpeg_len);
        return 1;
    }

    uint8_t *jpeg_buf = (uint8_t *)malloc(jpeg_len);
    if (!jpeg_buf) return 1;

    // 4) FIFO Verisini Çek
    arduchip_read_fifo_burst(&cam, jpeg_buf, jpeg_len);

    // 5) Dosyaya Kaydet
    FILE *fj = fopen("frame.jpg", "wb");
    if (fj) {
        fwrite(jpeg_buf, 1, jpeg_len, fj);
        fclose(fj);
        printf("Guncel goruntu frame.jpg olarak kaydedildi. Boyut: %u byte\n", jpeg_len);
    }

    // Temizlik
    free(jpeg_buf);
    i2c_close(i2c_cam);
    spi_close(spi_cam);

    return 0;
}

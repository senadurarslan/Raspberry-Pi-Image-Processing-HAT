#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "jpeg_to_rgb565.h"

int main(void)
{
    const char *in_filename  = "frame.jpg";
    const char *out_filename = "frame.rgb565";

    // JPEG dosyasını aç
    FILE *f = fopen(in_filename, "rb");
    if (!f) {
        perror("fopen(frame.jpg)");
        return 1;
    }

    // Boyutu öğren
    if (fseek(f, 0, SEEK_END) != 0) {
        perror("fseek end");
        fclose(f);
        return 1;
    }

    long file_size = ftell(f);
    if (file_size <= 0) {
        fprintf(stderr, "Geçersiz dosya boyutu: %ld\n", file_size);
        fclose(f);
        return 1;
    }

    rewind(f);

    // JPEG verisini RAM'e oku
    uint8_t *jpg_buf = (uint8_t *)malloc((size_t)file_size);
    if (!jpg_buf) {
        fprintf(stderr, "malloc(%ld) FAILED\n", file_size);
        fclose(f);
        return 1;
    }

    size_t read_bytes = fread(jpg_buf, 1, (size_t)file_size, f);
    fclose(f);

    if (read_bytes != (size_t)file_size) {
        fprintf(stderr, "fread: %zu/%ld byte\n", read_bytes, file_size);
        free(jpg_buf);
        return 1;
    }

    printf("JPEG okundu: %ld byte\n", file_size);

    // JPEG -> RGB565
    uint16_t *rgb565 = NULL;
    int width = 0, height = 0;

    int r = jpeg_to_rgb565(jpg_buf,
                           (size_t)file_size,
                           &rgb565,
                           &width,
                           &height);

    free(jpg_buf); // JPEG buffer artık gereksiz

    if (r != 0) {
        fprintf(stderr, "jpeg_to_rgb565() hata: %d\n", r);
        return 1;
    }

    printf("Decode OK: %dx%d, toplam piksel: %d, buffer: %zu byte\n",
           width, height, width * height,
           (size_t)width * (size_t)height * sizeof(uint16_t));

    // RGB565 ham veriyi dosyaya yaz
    FILE *out = fopen(out_filename, "wb");
    if (!out) {
        perror("fopen(frame.rgb565)");
        free(rgb565);
        return 1;
    }

    size_t written = fwrite(rgb565,
                            sizeof(uint16_t),
                            (size_t)width * (size_t)height,
                            out);
    fclose(out);

    if (written != (size_t)width * (size_t)height) {
        fprintf(stderr, "fwrite: %zu/%zu pixel\n",
                written, (size_t)width * (size_t)height);
        free(rgb565);
        return 1;
    }

    printf("RGB565 dosyaya yazıldı: %s\n", out_filename);

    // İleride TFT'ye direkt bu buffer'dan yazacağız
    free(rgb565);
    return 0;
}

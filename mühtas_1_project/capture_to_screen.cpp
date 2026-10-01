#include <iostream>
#include <vector>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>

// C k�t�phanesi i�in extern olu�turarak c++ kodunda kulland�k
extern "C" {
#include "spi_linux.h"
#include "i2c_linux.h"
#include "arduchip.h"
#include "ov2640.h"
#include "jpeg_to_rgb565.h"
}

// TFT k�t�hanesi
#include <ST7735_TFT_LCD_RDL.hpp>

// ArduChip Register Maskeleri
#define FIFO_CLEAR_MASK      0x01
#define FIFO_RDPTR_RST_MASK  0x10
#define FIFO_WRPTR_RST_MASK  0x20

ST7735_TFT myTFT;

int8_t RST_TFT      = 25;
int8_t DC_TFT       = 24;
int   GPIO_CHIP_DEV = 0;

uint8_t  OFFSET_COL = 0;
uint8_t  OFFSET_ROW = 0;
uint16_t TFT_WIDTH  = 128;
uint16_t TFT_HEIGHT = 160;

int SPI_DEV    = 0;
int SPI_CHANNEL= 1; 
int SPI_SPEED  = 8000000;
int SPI_FLAGS  = 0;

//================== Prototipler ==================
static uint8_t SetupDisplay();
static void    ShowErrorOnTFT(const char* msg);
static bool    CaptureJPEG(std::vector<uint8_t>& jpeg_buf, uint32_t& jpeg_len);
static bool    DecodeAndShow(const uint8_t* jpeg_data, uint32_t jpeg_len);

//================== Main =========================
int main()
{
    // 1. Ekranı hazırla
    if (SetupDisplay() != 0) {
        std::cerr << "TFT Kurulumu basarisiz!\n";
        return 1;
    }

    std::vector<uint8_t> jpeg_data;
    uint32_t jpeg_len = 0;

    // 2. Kameradan taze görüntü yakala
    std::cout << "Kameradan goruntu aliniyor...\n";
    if (!CaptureJPEG(jpeg_data, jpeg_len)) {
        std::cerr << "Kamera yakalama hatasi!\n";
        ShowErrorOnTFT("CAPTURE FAIL");
        myTFT.TFTPowerDown();
        return 1;
    }
    
        // --- İSTEDİĞİNİZ SATIR BURADA ---
    std::cout << "jpeg length : " << jpeg_len << " bytes" << std::endl;

    // 3. Çöz ve ekrana bas
    if (!DecodeAndShow(jpeg_data.data(), jpeg_len)) {
        std::cerr << "JPEG cozme hatasi!\n";
        ShowErrorOnTFT("DECODE FAIL");
        myTFT.TFTPowerDown();
        return 1;
    }

    std::cout << "Islem tamamlandi. Goruntu 5 saniye ekranda kalacak.\n";
    sleep(5);

    myTFT.TFTPowerDown();
    return 0;
}

//================== Donanım Fonksiyonları ====================

static uint8_t SetupDisplay()
{
    std::cout << "TFT Baslatiliyor...\n";
    myTFT.TFTSetupGPIO(RST_TFT, DC_TFT);
    myTFT.TFTInitScreenSize(OFFSET_COL, OFFSET_ROW, TFT_WIDTH, TFT_HEIGHT);

    if (myTFT.TFTInitPCBType(myTFT.TFT_ST7735R_Red, SPI_DEV, SPI_CHANNEL, SPI_SPEED, SPI_FLAGS, GPIO_CHIP_DEV) != rdlib::Success) {
        return 1;
    }

    delayMilliSecRDL(50);
    myTFT.TFTsetRotation(myTFT.Degrees_0);
    myTFT.fillScreen(myTFT.RDLC_BLACK);
    myTFT.setFont(font_pico);
    return 0;
}

static bool CaptureJPEG(std::vector<uint8_t>& jpeg_buf, uint32_t& jpeg_len)
{
    // SPI ve I2C açılışı
    int spi_cam = spi_open("/dev/spidev0.0", 8000000);
    if (spi_cam < 0) return false;

    struct arduchip cam;
    arduchip_init(&cam, spi_cam);

    int i2c_cam = i2c_open("/dev/i2c-1", OV2640_I2C_ADDR);
    if (i2c_cam < 0) { spi_close(spi_cam); return false; }

    // Kamera Ayarları
    ov2640_init(i2c_cam);
    ov2640_set_qqvga_jpeg(i2c_cam);
    sleep(1); // Sensörün ışığa alışması için bekleme

    // --- KRİTİK: DOUBLE CAPTURE MANTIĞI (Gecikmeyi önler) ---
    // 1. Sahte yakalama ile tamponu boşalt
    arduchip_start_capture(&cam);
    arduchip_wait_capture_done(&cam, 1000);
    arduchip_write_reg(&cam, ARDUCHIP_FIFO, FIFO_CLEAR_MASK);

    // 2. Donanım işaretçilerini sıfırla
    arduchip_write_reg(&cam, ARDUCHIP_FIFO, FIFO_RDPTR_RST_MASK);
    arduchip_write_reg(&cam, ARDUCHIP_FIFO, FIFO_WRPTR_RST_MASK);
    usleep(50000);

    // 3. Gerçek (Taze) yakalamayı başlat
    arduchip_start_capture(&cam);
    if (arduchip_wait_capture_done(&cam, 2000) < 0) {
        i2c_close(i2c_cam); spi_close(spi_cam); return false;
    }

    // Veri uzunluğunu al ve belleği hazırla
    jpeg_len = arduchip_read_fifo_length(&cam);
    if (jpeg_len == 0 || jpeg_len > 400 * 1024) {
        i2c_close(i2c_cam); spi_close(spi_cam); return false;
    }

    jpeg_buf.resize(jpeg_len);
    arduchip_read_fifo_burst(&cam, jpeg_buf.data(), jpeg_len);

    i2c_close(i2c_cam);
    spi_close(spi_cam);
    return true;
}

static bool DecodeAndShow(const uint8_t* jpeg_data, uint32_t jpeg_len)
{
    uint16_t* rgb565 = nullptr;
    int img_w = 0, img_h = 0;

    // JPEG -> RGB565 Çevrimi
    if (jpeg_to_rgb565(jpeg_data, jpeg_len, &rgb565, &img_w, &img_h) != 0 || !rgb565) {
        return false;
    }

    // Ekran ortalaması (Crop)
    int x0 = (img_w - TFT_WIDTH) / 2;
    int y0 = (img_h - TFT_HEIGHT) / 2;

    // Ekrana Piksel Piksel Yazma
    for (int y = 0; y < TFT_HEIGHT; ++y) {
        for (int x = 0; x < TFT_WIDTH; ++x) {
            // Kaynak resimden ilgili pikseli al
            uint16_t color = rgb565[(y0 + y) * img_w + (x0 + x)];
            
            // Not: Renkler tersse aşağıdaki satırı aktif et:
            // color = (color >> 8) | (color << 8);

            myTFT.drawPixel(x, y, color);
        }
    }

    free(rgb565);
    return true;
}

static void ShowErrorOnTFT(const char* msg)
{
    myTFT.fillScreen(myTFT.RDLC_BLACK);
    myTFT.setTextColor(myTFT.RDLC_RED, myTFT.RDLC_BLACK);
    myTFT.setCursor(5, 5);
    myTFT.println(msg);
}

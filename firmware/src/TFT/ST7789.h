#pragma once

#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "TFTDisplay.h"

struct Glyph {
    uint32_t unicode;       // Unicode value of the glyph
    int width;              // Width of the glyph bitmap bounding box
    int height;             // Height of the glyph bitmap bounding box
    int gxAdvance;          // Cursor advance after drawing this glyph
    int dX;                 // Distance from cursor to the left side of the glyph bitmap
    int dY;                 // Distance from the baseline to the top of the glyph bitmap
    const uint8_t* bitmap;  // Pointer to the glyph bitmap data
};

// Font data
struct Font {
    uint32_t gCount;        // Number of glyphs
    uint32_t ascent;        // Ascent in pixels from baseline to top of "d"
    uint32_t descent;       // Descent in pixels from baseline to bottom of "p"
    const uint8_t* fontData;  // Pointer to the raw VLW font data
};

class ST7789: public TFTDisplay {
public:
    QueueHandle_t transactionQueue;

    int width;
    int height;
    uint8_t rotation;

    ST7789(gpio_num_t mosi, gpio_num_t clk, gpio_num_t cs, gpio_num_t dc, gpio_num_t rst, gpio_num_t bl, int width = 240, int height = 320);
    ~ST7789();

   void startWrite(){
        spi_device_acquire_bus(spi, portMAX_DELAY);
    }
    void endWrite(){
        spi_device_release_bus(spi);
    }
    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
    void setWindow(int32_t x0, int32_t y0, int32_t x1, int32_t y1);
    void pushPixels(uint16_t *data, uint32_t len){
        sendPixels(data, len);
    }
    void pushPixelsDMA(uint16_t *data, uint32_t len) {
        sendPixels(data, len);
    }
    void dmaWait() {
        waitDMA();
    }
    void drawString(const char *string, int16_t x, int16_t y);
    void fillScreen(uint16_t color);
    void loadFont(const uint8_t *font);
    void setTextColor(uint16_t color, uint16_t bgColor);
    void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color);
private:
    void init();
    void sendCmd(uint8_t cmd);
    void sendData(const uint8_t *data, int len);
    void sendPixel(uint16_t color);
    void sendPixels(const uint16_t *data, int numPixels);
    void sendColor(uint16_t color, int numPixels);
    void waitDMA();
    void setRotation(uint8_t m);
    // Text rendering
    Glyph getGlyphData(uint32_t unicode);
    void drawPixel(uint16_t color, int x, int y);
    void drawGlyph(const Glyph& glyph, int x, int y);

    static void IRAM_ATTR spi_post_transfer_callback(spi_transaction_t *trans);
    static void IRAM_ATTR spi_pre_transfer_callback(spi_transaction_t *trans);

    gpio_num_t mosi;
    gpio_num_t clk;
    gpio_num_t cs;
    gpio_num_t dc;
    gpio_num_t rst;
    gpio_num_t bl;
    spi_device_handle_t spi;

    uint16_t textcolor;
    uint16_t textbgcolor;

    // The current font
    Font currentFont;
};

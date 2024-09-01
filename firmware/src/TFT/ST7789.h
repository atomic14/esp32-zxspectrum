#pragma once

#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "TFTDisplay.h"

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
    void drawString(const char *string, int16_t x, int16_t y) {}
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

    static void IRAM_ATTR spi_post_transfer_callback(spi_transaction_t *trans);
    static void IRAM_ATTR spi_pre_transfer_callback(spi_transaction_t *trans);

    gpio_num_t mosi;
    gpio_num_t clk;
    gpio_num_t cs;
    gpio_num_t dc;
    gpio_num_t rst;
    gpio_num_t bl;
    spi_device_handle_t spi;

    uint8_t *fontPtr = nullptr;
    uint16_t textcolor;
    uint16_t textbgcolor;
};

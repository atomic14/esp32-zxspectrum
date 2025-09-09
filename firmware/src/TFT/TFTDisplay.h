#pragma once
#include "Display.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "../Serial.h"

#define SEND_CMD_DATA(cmd, data...)        \
{                                          \
    const uint8_t c = cmd, x[] = {data};   \
    sendCmd(c);                            \
    if (sizeof(x))                         \
        sendData(x, sizeof(x));            \
}

#define ST7789_CMD_CASET 0x2A
#define ST7789_CMD_RASET 0x2B
#define ST7789_CMD_RAMWR 0x2C

#define TFT_WHITE   0xFFFF
#define TFT_BLACK   0x0000
#define TFT_RED     0xF800
#define TFT_GREEN   0x07E0
#define TFT_BLUE    0x001F
#define TFT_CYAN    0x07FF
#define TFT_YELLOW  0xFFE0
#define TFT_MAGENTA 0xF81F

#define TFT_NOP     0x00
#define TFT_SWRST   0x01

#define TFT_SLPIN   0x10
#define TFT_SLPOUT  0x11
#define TFT_NORON   0x13

#define TFT_INVOFF  0x20
#define TFT_INVON   0x21
#define TFT_DISPOFF 0x28
#define TFT_DISPON  0x29
#define TFT_CASET   0x2A
#define TFT_PASET   0x2B
#define TFT_RAMWR   0x2C
#define TFT_RAMRD   0x2E
#define TFT_MADCTL  0x36
#define TFT_COLMOD  0x3A

// Flags for TFT_MADCTL
#define TFT_MAD_MY  0x80
#define TFT_MAD_MX  0x40
#define TFT_MAD_MV  0x20
#define TFT_MAD_ML  0x10
#define TFT_MAD_RGB 0x00
#define TFT_MAD_BGR 0x08
#define TFT_MAD_MH  0x04
#define TFT_MAD_SS  0x02
#define TFT_MAD_GS  0x01


class SPITransactionInfo;

class TFTDisplay : public Display
{
public:
  TFTDisplay(gpio_num_t cs, gpio_num_t dc, gpio_num_t rst, gpio_num_t bl, int width, int height);
  void setWindow(int32_t x0, int32_t y0, int32_t x1, int32_t y1);
  void dmaWait();
  void startWrite() {
    xSemaphoreTake(mDisplayLock, portMAX_DELAY);
    dmaWait();
    spi_device_acquire_bus(spi, portMAX_DELAY);
  }
  void endWrite() {
    dmaWait();
    spi_device_release_bus(spi);
    xSemaphoreGive(mDisplayLock);
  }
protected:
  void init();
  void sendCmd(uint8_t cmd);
  void sendData(const uint8_t *data, int len);
  void sendPixel(uint16_t color);
  void sendPixels(const uint16_t *data, int numPixels);
  void sendColor(uint16_t color, int numPixels);

  volatile bool isBusy = false;
  SPITransactionInfo *_transaction;
  void sendTransaction(SPITransactionInfo *trans);

  gpio_num_t cs;
  gpio_num_t dc;
  gpio_num_t rst;
  gpio_num_t bl;
  spi_device_handle_t spi;
  SemaphoreHandle_t mDisplayLock;
};
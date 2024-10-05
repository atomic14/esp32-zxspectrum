#pragma once

#include <TFT_eSPI.h>

class TFTDisplay {
public:
  TFTDisplay() {};
  virtual void startWrite() = 0;
  virtual void endWrite() = 0;
  virtual void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) = 0;
  virtual void setWindow(int32_t x0, int32_t y0, int32_t x1, int32_t y1) = 0;
  virtual void pushPixels(uint16_t *data, uint32_t len) = 0;
  virtual void pushPixelsDMA(uint16_t *data, uint32_t len) = 0;
  virtual void dmaWait() = 0;
  virtual void drawString(const char *string, int16_t x, int16_t y) = 0;
  virtual void fillScreen(uint16_t color) = 0;
  virtual void loadFont(const uint8_t *font) = 0;
  virtual void setTextColor(uint16_t color, uint16_t bgColor) = 0;
  virtual void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) = 0;
  virtual uint16_t color565(uint8_t r, uint8_t g, uint8_t b) = 0;
  virtual int width() = 0;
  virtual int height() = 0;
};
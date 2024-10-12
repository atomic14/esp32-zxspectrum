#pragma once
#include <vector>
#include <TFT_eSPI.h>

#define SWAPBYTES(i) ((i >> 8) | (i << 8))

struct Point {
    int16_t x;
    int16_t y;
};

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
  virtual void drawString(const char *string, int16_t x, int16_t y);
  virtual void fillScreen(uint16_t color) = 0;
  virtual void loadFont(const uint8_t *font) = 0;
  virtual void setTextColor(uint16_t color, uint16_t bgColor) = 0;
  virtual void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) = 0;
  virtual void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) = 0;
  virtual void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) = 0;
  virtual void drawPolygon(const std::vector<Point>& vertices, uint16_t color) = 0;
  virtual void drawFilledPolygon(const std::vector<Point>& vertices, uint16_t color) = 0;
  virtual uint16_t color565(uint8_t r, uint8_t g, uint8_t b) { 
    // Convert 8-8-8 RGB to 5-6-5 RGB
    uint16_t r2 = r >> 3;
    uint16_t g2 = g >> 2;
    uint16_t b2 = b >> 3;
    return (r2 << 11) | (g2 << 5) | b2;
  }
  virtual int width() { return TFT_WIDTH;}
  virtual int height() { return TFT_HEIGHT; }
};
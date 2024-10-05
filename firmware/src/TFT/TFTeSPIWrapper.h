#pragma once

#include "TFTDisplay.h"

class TFT_eSPI;
class TFTeSPIWrapper : public TFTDisplay
{
private:
  TFT_eSPI *tft;

public:
  TFTeSPIWrapper()
  {
#ifdef TFT_POWER
    // turn on the TFT
    pinMode(TFT_POWER, OUTPUT);
    digitalWrite(TFT_POWER, LOW);
#endif
    tft = new TFT_eSPI();
    tft->begin();
#ifdef USE_DMA
    tft->initDMA(); // Initialise the DMA engine
#endif
#ifdef TFT_ROTATION
    tft->setRotation(TFT_ROTATION);
#else
    tft->setRotation(3);
#endif
    tft->fillScreen(TFT_BLACK);
  }
  void startWrite() {
    tft->startWrite();
  }
  void endWrite() {
    tft->endWrite();
  }
  void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    tft->fillRect(x, y, w, h, color);
  }
  void setWindow(int32_t x0, int32_t y0, int32_t x1, int32_t y1) {
    tft->setWindow(x0, y0, x1, y1);
  }
  void pushPixels(uint16_t *data, uint32_t len) {
    tft->pushPixels(data, len);
  }
  void pushPixelsDMA(uint16_t *data, uint32_t len) {
    tft->pushPixelsDMA(data, len);
  }
  void dmaWait() {
    tft->dmaWait();
  }
  void drawString(const char *string, int16_t x, int16_t y) {
    tft->drawString(string, x, y);
  }
  void fillScreen(uint16_t color) {
    tft->fillScreen(color);
  }
  void loadFont(const uint8_t *font) {
    tft->loadFont(font);
  
  }
  void setTextColor(uint16_t color, uint16_t bgColor) {
    tft->setTextColor(color, bgColor);
  }
  void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) {
    tft->drawFastHLine(x, y, w, color);
  }
  inline uint16_t color565(uint8_t r, uint8_t g, uint8_t b) {
    return tft->color565(r, g, b);
  }
  inline int width() {
    return tft->width();
  }
  inline int height() {
    return tft->height();
  }
};
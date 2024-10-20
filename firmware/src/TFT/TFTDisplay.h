#pragma once
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include <vector>

#define TFT_WHITE 0xFFFF
#define TFT_BLACK 0x0000
#define TFT_RED 0xF800
#define TFT_GREEN 0x07E0

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

#define SEND_CMD_DATA(cmd, data...)        \
{                                          \
    const uint8_t c = cmd, x[] = {data};   \
    sendCmd(c);                            \
    if (sizeof(x))                         \
        sendData(x, sizeof(x));            \
}

#define SWAPBYTES(i) ((i >> 8) | (i << 8))

struct Point
{
  int16_t x;
  int16_t y;
};

struct Glyph
{
  uint32_t unicode;      // Unicode value of the glyph
  int16_t width;         // Width of the glyph bitmap bounding box
  int16_t height;        // Height of the glyph bitmap bounding box
  int16_t gxAdvance;     // Cursor advance after drawing this glyph
  int16_t dX;            // Distance from cursor to the left side of the glyph bitmap
  int16_t dY;            // Distance from the baseline to the top of the glyph bitmap
  const uint8_t *bitmap; // Pointer to the glyph bitmap data
};

// Font data
struct Font
{
  uint32_t gCount;         // Number of glyphs
  uint32_t ascent;         // Ascent in pixels from baseline to top of "d"
  uint32_t descent;        // Descent in pixels from baseline to bottom of "p"
  const uint8_t *fontData; // Pointer to the raw VLW font data
};

class SPITransactionInfo;

class TFTDisplay
{
public:
  TFTDisplay(gpio_num_t mosi, gpio_num_t clk, gpio_num_t cs, gpio_num_t dc, gpio_num_t rst, gpio_num_t bl, int width, int height);
  void startWrite()
  {
    // nothing to do
  }
  void endWrite()
  {
    // nothing to do
  }
  void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
  void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
  void setWindow(int32_t x0, int32_t y0, int32_t x1, int32_t y1);
  void pushPixels(uint16_t *data, uint32_t len)
  {
    sendPixels(data, len);
  }
  void pushPixelsDMA(uint16_t *data, uint32_t len)
  {
    sendPixels(data, len);
  }
  void dmaWait();
  void drawString(const char *string, int16_t x, int16_t y);
  Point measureString(const char *string);
  void fillScreen(uint16_t color);
  void loadFont(const uint8_t *font);
  void setTextColor(uint16_t color, uint16_t bgColor);
  void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color);
  void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color);
  void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
  void drawPolygon(const std::vector<Point> &vertices, uint16_t color);
  void drawFilledPolygon(const std::vector<Point> &vertices, uint16_t color);
  static uint16_t color565(uint8_t r, uint8_t g, uint8_t b)
  {
    // Convert 8-8-8 RGB to 5-6-5 RGB
    uint16_t r2 = r >> 3;
    uint16_t g2 = g >> 2;
    uint16_t b2 = b >> 3;
    return (r2 << 11) | (g2 << 5) | b2;
  }
  virtual int width() { return _width; }
  virtual int height() { return _height; }

protected:
  int _width;
  int _height;

  void init();
  void sendCmd(uint8_t cmd);
  void sendData(const uint8_t *data, int len);
  void sendPixel(uint16_t color);
  void sendPixels(const uint16_t *data, int numPixels);
  void sendColor(uint16_t color, int numPixels);

  volatile bool isBusy = false;
  SPITransactionInfo *_transaction;
  void sendTransaction(SPITransactionInfo *trans);

  // Text rendering
  Glyph getGlyphData(uint32_t unicode);
  void drawPixel(uint16_t color, int x, int y);
  void drawGlyph(const Glyph &glyph, int x, int y);

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
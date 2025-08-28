#pragma once

#include <cstdint>
#include <vector>

#define TFT_WHITE 0xFFFF
#define TFT_BLACK 0x0000
#define TFT_RED 0xF800
#define TFT_GREEN 0x07E0

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

// Abstract class for a display
class Display
{
public:
  Display(int width, int height) : _width(width), _height(height) {}
  static inline uint16_t swapBytes(uint16_t val)
  {
    return (val >> 8) | (val << 8);
  }
  virtual void startWrite()
  {
    // nothing to do
  }
  virtual void endWrite()
  {
    // nothing to do
  }
  virtual void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
  virtual void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
  virtual void setWindow(int32_t x0, int32_t y0, int32_t x1, int32_t y1) = 0;
  virtual void pushPixels(uint16_t *data, uint32_t len)
  {
    sendPixels(data, len);
  }
  virtual void pushPixelsDMA(uint16_t *data, uint32_t len)
  {
    sendPixels(data, len);
  }
  virtual void dmaWait() {};
  virtual void drawString(const char *string, int16_t x, int16_t y);
  virtual Point measureString(const char *string);
  virtual void fillScreen(uint16_t color);
  virtual void loadFont(const uint8_t *font);
  virtual void setTextColor(uint16_t color, uint16_t bgColor);
  virtual void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color);
  virtual void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color);
  virtual void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
  virtual void drawPolygon(const std::vector<Point> &vertices, uint16_t color);
  virtual void drawFilledPolygon(const std::vector<Point> &vertices, uint16_t color);
  static uint16_t color565(uint8_t r, uint8_t g, uint8_t b)
  {
    // Convert 8-8-8 RGB to 5-6-5 RGB
    uint16_t r2 = r >> 3;
    uint16_t g2 = g >> 2;
    uint16_t b2 = b >> 3;
    return (r2 << 11) | (g2 << 5) | b2;
  }
  int width() { return _width; }
  int height() { return _height; }
  virtual void saveScreenshot(uint16_t borderWidth = 20, uint16_t borderColor = TFT_BLACK) {
    // not implemented here - maybe somewhere else?
  }
  virtual void drawPixel(uint16_t color, int x, int y);
  // Draw a centered string at y
  void drawCenterString(const char *string, int16_t y){
    Point menuSize = measureString(string);
    int centerX = (_width - menuSize.x) / 2;
    drawString(string, centerX, y);
  }
  // Draw a string and return the rightmost x
  int16_t drawStringAndMeasure(const char *string, int16_t x, int16_t y){
    Point size = measureString(string);
    drawString(string, x, y);
    return x + size.x;
  }
protected:
  int _width;
  int _height;

  virtual void sendPixel(uint16_t color) = 0;
  virtual void sendPixels(const uint16_t *data, int numPixels) = 0;
  virtual void sendColor(uint16_t color, int numPixels) = 0;

  // Text rendering
  virtual Glyph getGlyphData(uint32_t unicode);
  virtual void drawGlyph(const Glyph &glyph, int x, int y);

  uint16_t textcolor;
  uint16_t textbgcolor;

  // The current font
  Font currentFont;
};
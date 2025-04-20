#include <Arduino.h>
#include "Serial.h"
#include "Display.h"
#include <cstring>
#include <algorithm>
#include <vector>

uint32_t swapEndian32(uint32_t val)
{
  return ((val >> 24) & 0x000000FF) |
         ((val >> 8) & 0x0000FF00) |
         ((val << 8) & 0x00FF0000) |
         ((val << 24) & 0xFF000000);
}

int32_t readInt32(const uint8_t *data)
{
  int32_t val = *((int32_t *)data);
  val = (int32_t)swapEndian32((uint32_t)val);
  return val;
}

uint32_t readUInt32(const uint8_t *data)
{
  uint32_t val = *((uint32_t *)data);
  val = (uint32_t)swapEndian32((uint32_t)val);
  return val;
}


void Display::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
  setWindow(x, y, x + w - 1, y + h - 1);
  sendColor(SWAPBYTES(color), w * h);
}

void Display::drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
  color = SWAPBYTES(color);
  drawFastHLine(x, y, w, color);
  drawFastHLine(x, y + h - 1, w, color);
  drawFastVLine(x, y, h, color);
  drawFastVLine(x + w - 1, y, h, color);
}

void Display::drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color)
{
  fillRect(x, y, w, 1, swapBytes(color));
}

void Display::drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color)
{
  fillRect(x, y, 1, h, swapBytes(color));
}

void Display::drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color)
{
  // make sure that the line is always drawn from left to right
  if (x0 > x1)
  {
    std::swap(x0, x1);
    std::swap(y0, y1);
  }
  int16_t dx = abs(x1 - x0);
  int16_t dy = abs(y1 - y0);
  int16_t sx = (x0 < x1) ? 1 : -1;
  int16_t sy = (y0 < y1) ? 1 : -1;
  int16_t err = dx - dy;
  int16_t e2;

  while (true)
  {
    // Draw the current pixel
    drawPixel(color, x0, y0);

    // If we've reached the end of the line, break out of the loop
    if (x0 == x1 && y0 == y1)
    {
      break;
    }

    e2 = err * 2;

    if (e2 > -dy)
    { // Horizontal step
      err -= dy;
      x0 += sx;
    }

    if (e2 < dx)
    { // Vertical step
      err += dx;
      y0 += sy;
    }
  }
}

void Display::drawPolygon(const std::vector<Point> &vertices, uint16_t color)
{
  if (vertices.size() < 3)
  {
    return; // Not a polygon
  }
  for (size_t i = 0; i < vertices.size(); i++)
  {
    const Point &v1 = vertices[i];
    const Point &v2 = vertices[(i + 1) % vertices.size()];
    drawLine(v1.x, v1.y, v2.x, v2.y, color);
  }
}

void Display::drawFilledPolygon(const std::vector<Point> &vertices, uint16_t color)
{
  if (vertices.size() < 3)
  {
    return; // Not a polygon
  }

  // Find the bounding box of the polygon
  int16_t minY = vertices[0].y;
  int16_t maxY = vertices[0].y;
  for (const auto &vertex : vertices)
  {
    minY = std::min(minY, vertex.y);
    maxY = std::max(maxY, vertex.y);
  }

  // Scanline fill algorithm
  for (int16_t y = minY; y <= maxY; y++)
  {
    std::vector<int16_t> nodeX;

    // Find intersections of the scanline with polygon edges
    for (size_t i = 0; i < vertices.size(); i++)
    {
      Point v1 = vertices[i];
      Point v2 = vertices[(i + 1) % vertices.size()];

      if ((v1.y < y && v2.y >= y) || (v2.y < y && v1.y >= y))
      {
        int16_t x = v1.x + (y - v1.y) * (v2.x - v1.x) / (v2.y - v1.y);
        nodeX.push_back(x);
      }
    }

    // Sort the intersection points
    std::sort(nodeX.begin(), nodeX.end());

    // Draw horizontal lines between pairs of intersections
    for (size_t i = 0; i < nodeX.size(); i += 2)
    {
      if (i + 1 < nodeX.size())
      {
        drawFastHLine(nodeX[i], y, nodeX[i + 1] - nodeX[i] + 1, color);
      }
    }
  }
}

void Display::fillScreen(uint16_t color)
{
  fillRect(0, 0, _width, _height, color);
}

void Display::loadFont(const uint8_t *fontData)
{
  if (currentFont.fontData == fontData)
  {
    return;
  }
  currentFont.fontData = fontData;
  // Read font metadata with endianness correction
  currentFont.gCount = readUInt32(fontData);
  uint32_t version = readUInt32(fontData + 4);
  uint32_t fontSize = readUInt32(fontData + 8);
  uint32_t mboxY = readUInt32(fontData + 12);
  currentFont.ascent = readUInt32(fontData + 16);
  currentFont.descent = readUInt32(fontData + 20);
}

void Display::setTextColor(uint16_t color, uint16_t bgColor)
{
  textcolor = color;
  textbgcolor = bgColor;
}

Glyph Display::getGlyphData(uint32_t unicode)
{
  const uint8_t *fontPtr = currentFont.fontData + 24;
  const uint8_t *bitmapPtr = currentFont.fontData + 24 + currentFont.gCount * 28;

  for (uint32_t i = 0; i < currentFont.gCount; i++)
  {
    uint32_t glyphUnicode = readUInt32(fontPtr);
    uint32_t height = readUInt32(fontPtr + 4);
    uint32_t width = readUInt32(fontPtr + 8);
    int32_t gxAdvance = readInt32(fontPtr + 12);
    int32_t dY = readInt32(fontPtr + 16);
    int32_t dX = readInt32(fontPtr + 20);

    if (glyphUnicode == unicode)
    {
      Glyph glyph;
      glyph.unicode = glyphUnicode;
      glyph.width = width;
      glyph.height = height;
      glyph.gxAdvance = gxAdvance;
      glyph.dX = dX;
      glyph.dY = dY;
      glyph.bitmap = bitmapPtr;
      return glyph;
    }

    // Move to next glyph (28 bytes for metadata + bitmap size)
    fontPtr += 28;
    bitmapPtr += width * height;
  }

  // Return default glyph if not found
  Serial.printf("Glyph not found: %c\n", unicode);
  return getGlyphData(' ');
}

void Display::drawPixel(uint16_t color, int x, int y)
{
  if (x < 0 || x >= _width || y < 0 || y >= _height)
  {
    return;
  }
  setWindow(x, y, x, y);
  sendPixel(swapBytes(color));
}

void Display::drawGlyph(const Glyph &glyph, int x, int y)
{
  // Iterate over each pixel in the glyph's bitmap
  const uint8_t *bitmap = glyph.bitmap;
  uint16_t pixelBuffer[glyph.width * glyph.height] = {textbgcolor};
  for (int j = 0; j < glyph.height; j++)
  {
    for (int i = 0; i < glyph.width; i++)
    {
      // Get the alpha value for the current pixel (1 byte per pixel)
      uint8_t alpha = bitmap[j * glyph.width + i];
      // blend between the text color and the background color
      uint16_t fg = textcolor;
      uint16_t bg = textbgcolor;
      // extract the red, green and blue
      uint8_t fgRed = (fg >> 11) & 0x1F;
      uint8_t fgGreen = (fg >> 5) & 0x3F;
      uint8_t fgBlue = fg & 0x1F;

      uint8_t bgRed = (bg >> 11) & 0x1F;
      uint8_t bgGreen = (bg >> 5) & 0x3F;
      uint8_t bgBlue = bg & 0x1F;

      uint8_t red = ((fgRed * alpha) + (bgRed * (255 - alpha))) / 255;
      uint8_t green = ((fgGreen * alpha) + (bgGreen * (255 - alpha))) / 255;
      uint8_t blue = ((fgBlue * alpha) + (bgBlue * (255 - alpha))) / 255;

      uint16_t color = (red << 11) | (green << 5) | blue;
      pixelBuffer[i + j * glyph.width] = swapBytes(color);
    }
  }
  setWindow(x + glyph.dX, y - glyph.dY, x + glyph.dX + glyph.width - 1, y + glyph.dY + glyph.height - 1);
  sendPixels(pixelBuffer, glyph.width * glyph.height);
}

void Display::drawString(const char *text, int16_t x, int16_t y)
{
  int cursorX = x;
  int cursorY = y;

  while (*text)
  {
    char c = *text++;

    // Get the glyph data for the character
    Glyph glyph = getGlyphData((uint32_t)c);

    // Draw the glyph bitmap at the current cursor position
    drawGlyph(glyph, cursorX, cursorY + currentFont.ascent);

    // Move the cursor to the right by the glyph's gxAdvance value
    cursorX += glyph.gxAdvance;
  }
}

Point Display::measureString(const char *string)
{
  Point result = {0, 0};
  int cursorX = 0;
  int cursorY = 0;
  while (*string)
  {
    char c = *string++;

    // Get the glyph data for the character
    Glyph glyph = getGlyphData((uint32_t)c);

    // Move the cursor to the right by the glyph's gxAdvance value
    cursorX += glyph.gxAdvance;
    result.y = std::max(result.y, glyph.height);
  }
  result.x = cursorX;
  return result;
}

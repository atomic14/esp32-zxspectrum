#pragma once
#include "Display.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "../Serial.h"

class FrameBufferDisplay : public Display
{
public:
  FrameBufferDisplay(gpio_num_t mosi, gpio_num_t clk, gpio_num_t cs, gpio_num_t dc, int width, int height);
  void setWindow(int32_t x0, int32_t y0, int32_t x1, int32_t y1);
  void flush();
  bool sendSpectrum(uint8_t *spectrumDisplay, uint8_t *borderColors);
protected:
  void init();
  void sendCmd(uint8_t cmd);
  void sendData(const uint8_t *data, int len);
  void sendPixel(uint16_t color);
  void sendPixels(const uint16_t *data, int numPixels);
  void sendColor(uint16_t color, int numPixels);

  gpio_num_t mosi;
  gpio_num_t clk;
  gpio_num_t cs;
  gpio_num_t dc;
  spi_device_handle_t spi;

  // Frame buffer rendering
  int screenshotCount = 0;
  uint16_t framebuffer[320 * 240] = {0}; // Full display framebuffer
  bool isRowDirty[240] = {false};
  int32_t windowX0, windowY0, windowX1, windowY1; // Active window
  int32_t currentX, currentY; // Current pixel being written

  void writePixelsToFrameBuffer(uint16_t color, size_t count) {
    for (size_t i = 0; i < count; ++i) {
        if (currentX >= 0 && currentX < 320 && currentY >= 0 && currentY < 240) {
            framebuffer[currentY * 320 + currentX] = SWAPBYTES(color); // Write to framebuffer
        }

        // Move to the next pixel in the window
        currentX++;
        if (currentX > windowX1) {
            currentX = windowX0;
            currentY++;
        }
    }
  }

  void writePixelsToFrameBuffer(const uint16_t *buffer, size_t count) {
    for (size_t i = 0; i < count; ++i) {
        if (currentX >= 0 && currentX < 320 && currentY >= 0 && currentY < 240) {
            framebuffer[currentY * 320 + currentX] = SWAPBYTES(buffer[i]); // Write pixel from buffer
        }

        // Move to the next pixel in the window
        currentX++;
        if (currentX > windowX1) {
            currentX = windowX0;
            currentY++;
        }
    }
  }
};
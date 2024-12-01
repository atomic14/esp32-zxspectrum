#include <Arduino.h>
#include "Serial.h"
#include "FrameBufferDisplay.h"
#include <cstring>
#include <algorithm>
#include <vector>

  // spi_transaction_t transaction;

  // bool setData(const uint8_t *data, int len)
  // {
  //   memset(&transaction, 0, sizeof(transaction));
  //   isCommand = false;
  //   transaction.length = len * 8; // Data length in bits
  //   transaction.user = this;
  //   if (len <= 4)
  //   {
  //     for (int i = 0; i < len; i++)
  //     {
  //       transaction.tx_data[i] = data[i];
  //     }
  //     transaction.flags = SPI_TRANS_USE_TXDATA | SPI_TRANS_USE_RXDATA;
  //   }
  //   else
  //   {
  //     memcpy(buffer, data, len);
  //     transaction.tx_buffer = buffer;
  //   }
  //   return true;
  // }


FrameBufferDisplay::FrameBufferDisplay(gpio_num_t mosi, gpio_num_t clk, gpio_num_t cs, int width, int height)
    : Display(width, height), mosi(mosi), clk(clk), cs(cs), spi(nullptr)
{
}

void FrameBufferDisplay::sendPixels(const uint16_t *data, int numPixels)
{
  writePixelsToFrameBuffer(data, numPixels);
}

void FrameBufferDisplay::sendColor(uint16_t color, int numPixels)
{
  writePixelsToFrameBuffer(color, numPixels);
}

void FrameBufferDisplay::setWindow(int32_t x0, int32_t y0, int32_t x1, int32_t y1)
{
  // store the window for future frame buffer updates
  windowX0 = x0;
  windowY0 = y0;
  windowX1 = x1;
  windowY1 = y1;
  currentX = x0;
  currentY = y0;
}

void FrameBufferDisplay::sendPixel(uint16_t color)
{
  uint16_t data[1];
  data[0] = color;
  sendPixels(data, 1);
}

void FrameBufferDisplay::flush()
{
  for(int y = 0; y < _height; y++) {
    if(isRowDirty[y]) {
      spi_transaction_t transaction;
      memset(&transaction, 0, sizeof(transaction));
      transaction.length = _width * 2;
      transaction.tx_buffer = (uint8_t *) (framebuffer + y * _width * 2);
      spi_device_polling_transmit(spi, &transaction);
      isRowDirty[y] = false;
    }
  }
}


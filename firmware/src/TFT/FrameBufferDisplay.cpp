#include <Arduino.h>
#include "Serial.h"
#include "FrameBufferDisplay.h"
#include <cstring>
#include <algorithm>
#include <vector>
#include <SPI.h>


FrameBufferDisplay::FrameBufferDisplay(gpio_num_t mosi, gpio_num_t clk, gpio_num_t cs, int width, int height)
    : Display(width, height), mosi(mosi), clk(clk), cs(cs), spi(nullptr)
{
  Serial.println("Starting up SPI");
  SPI.begin(clk, 5, mosi, cs);
  SPI.setFrequency(12000000);
  pinMode(cs, OUTPUT);
  digitalWrite(cs, HIGH);  // CS is active LOW

  // // Initialize SPI
  // spi_bus_config_t buscfg = {
  //     .mosi_io_num = mosi,
  //     .miso_io_num = -1,
  //     .sclk_io_num = clk,
  //     .quadwp_io_num = -1,
  //     .quadhd_io_num = -1,
  //     .data4_io_num = -1,
  //     .data5_io_num = -1,
  //     .data6_io_num = -1,
  //     .data7_io_num = -1,
  //     .max_transfer_sz = 65535,
  //     .flags = SPICOMMON_BUSFLAG_MASTER,
  //     //.isr_cpu_id = ESP_INTR_CPU_AFFINITY_1,
  //     .intr_flags = 0,
  // };

  // spi_device_interface_config_t devcfg = {
  //     .command_bits = 0,
  //     .address_bits = 0,
  //     .dummy_bits = 0,
  //     .mode = 0, // SPI mode 0
  //     //.clock_source = SPI_CLK_SRC_DEFAULT,                   // Use the same frequency as the APB bus
  //     .duty_cycle_pos = 128,
  //     .cs_ena_pretrans = 0,
  //     .cs_ena_posttrans = 0,
  //     .clock_speed_hz = 1000000,
  //     .input_delay_ns = 0,
  //     .spics_io_num = cs, // CS pin
  //     .flags = SPI_DEVICE_NO_DUMMY,
  //     .queue_size = 1,
  //     .pre_cb = nullptr,
  //     .post_cb = nullptr

  // };

  // ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));
  // ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &devcfg, &spi));

  // spi_device_acquire_bus(spi, portMAX_DELAY);
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

  // set the dirty rows to true
  for(int y = y0; y <= y1; y++) {
    isRowDirty[y] = true;
  }
}

void FrameBufferDisplay::sendPixel(uint16_t color)
{
  uint16_t data[1];
  data[0] = color;
  sendPixels(data, 1);
}

void FrameBufferDisplay::flush()
{
  // rowIndex + rowBytes + checksum
  uint8_t *dmaBuffer = (uint8_t *)heap_caps_malloc(_width * 2 + 2, MALLOC_CAP_DMA);
  for(int y = 0; y < _height; y++) {
    if(isRowDirty[y]) {
      dmaBuffer[0] = y;
      memcpy(dmaBuffer + 1, framebuffer + y * _width, _width * 2);
      // compute the checksum
      uint8_t checksum = 0;
      for(int i = 0; i < _width * 2 + 1; i++) {
        checksum ^= dmaBuffer[i];
      }
      dmaBuffer[_width * 2 + 1] = checksum;
      // spi_transaction_t transaction;
      // memset(&transaction, 0, sizeof(transaction));
      // transaction.length = _width * 2 + 2;
      // transaction.tx_buffer = dmaBuffer;
      // spi_device_polling_transmit(spi, &transaction);
      digitalWrite(cs, LOW);
      SPI.writeBytes(dmaBuffer, _width * 2 + 2);
      digitalWrite(cs, HIGH);
      isRowDirty[y] = false;
    }
  }
  heap_caps_free(dmaBuffer);
}


#include <Arduino.h>
#include "Serial.h"
#include "FrameBufferDisplay.h"
#include <cstring>
#include <algorithm>
#include <vector>
#include <SPI.h>


FrameBufferDisplay::FrameBufferDisplay(gpio_num_t mosi, gpio_num_t clk, gpio_num_t cs, gpio_num_t dc, int width, int height)
    : Display(width, height), mosi(mosi), clk(clk), cs(cs), dc(dc), spi(nullptr)
{
  Serial.println("Starting up SPI");
  // Initialize SPI
  spi_bus_config_t buscfg = {
      .mosi_io_num = mosi,
      .miso_io_num = -1,
      .sclk_io_num = clk,
      .quadwp_io_num = -1,
      .quadhd_io_num = -1,
      .data4_io_num = -1,
      .data5_io_num = -1,
      .data6_io_num = -1,
      .data7_io_num = -1,
      .max_transfer_sz = 65535,
      .flags = SPICOMMON_BUSFLAG_MASTER,
      //.isr_cpu_id = ESP_INTR_CPU_AFFINITY_1,
      .intr_flags = 0,
  };

  spi_device_interface_config_t devcfg = {
      .command_bits = 0,
      .address_bits = 0,
      .dummy_bits = 0,
      .mode = 0, // SPI mode 0
      //.clock_source = SPI_CLK_SRC_DEFAULT,                   // Use the same frequency as the APB bus
      .duty_cycle_pos = 128,
      .cs_ena_pretrans = 0,
      .cs_ena_posttrans = 0,
      .clock_speed_hz = 10000000,
      .input_delay_ns = 0,
      .spics_io_num = cs, // CS pin
      .flags = SPI_DEVICE_NO_DUMMY,
      .queue_size = 1,
      .pre_cb = nullptr,
      .post_cb = nullptr
  };

  ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));
  ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &devcfg, &spi));

  spi_device_acquire_bus(spi, portMAX_DELAY);
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

bool FrameBufferDisplay::sendSpectrum(uint8_t *spectrumDisplay, uint8_t *borderColors)
{
  digitalWrite(dc, HIGH);
  spi_transaction_t transaction;
  memset(&transaction, 0, sizeof(transaction));
  // something freaky is going on with the SPI on the PI - it occasionally seems to be offset by 8 bytes
  // we'll fix this by adding on 8 bytes and a marker byte of 0xFF
  uint8_t *dmaBuffer = (uint8_t *)heap_caps_malloc(6912 + 240 + 9, MALLOC_CAP_DMA);
  if (dmaBuffer == nullptr) {
    Serial.println("Failed to allocate DMA Buffer");
  }
  dmaBuffer[0] = 0xFF;
  memcpy(dmaBuffer + 1, spectrumDisplay, 6912);
  memcpy(dmaBuffer + 6912 + 1, borderColors + (312 - 240)/2, 240);
  transaction.length = (6912 + 240 + 9) * 8;
  transaction.tx_buffer = dmaBuffer;
  spi_device_polling_transmit(spi, &transaction);
  heap_caps_free(dmaBuffer);
  return true;
}

void FrameBufferDisplay::flush()
{
  return;
  uint8_t *dmaBuffer = (uint8_t *)heap_caps_malloc(6912 + 240 + 9, MALLOC_CAP_DMA);
  int index = 0;
  dmaBuffer[0] = 0xFE; // marker byte for start of flush
  dmaBuffer[1] = 0; // this will be the number of rows sent
  int offset = 2;
  for(int y = 0; y < _height; y++) {
    if(isRowDirty[y]) {
      dmaBuffer[offset] = y; // row number
      offset++;
      memcpy(dmaBuffer + offset, framebuffer + y * _width, _width * 2);
      offset += _width * 2;
      index++;
      // we can fit 10 rows in the buffer
      if (index == 10) {
        Serial.println("Sending 10 rows");
        dmaBuffer[1] = index; // number of rows sent
        spi_transaction_t transaction;
        memset(&transaction, 0, sizeof(transaction));
        transaction.length = (6912 + 240 + 9) * 8;
        transaction.tx_buffer = dmaBuffer;
        spi_device_polling_transmit(spi, &transaction);
        index = 0;
        offset = 2;
        vTaskDelay(1);
      }
    }
  }
  // send any remaining rows
  if (index > 0) {
    Serial.printf("Sending last %d rows\n", index);
    dmaBuffer[1] = index; // number of rows sent
    spi_transaction_t transaction;
    memset(&transaction, 0, sizeof(transaction));
    transaction.length = (6912 + 240 + 9) * 8;
    transaction.tx_buffer = dmaBuffer;
    spi_device_polling_transmit(spi, &transaction);
  }
  heap_caps_free(dmaBuffer);
}


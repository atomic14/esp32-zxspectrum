#include <Arduino.h>
#include "Serial.h"
#include "HDMIDisplay.h"
#include <cstring>
#include <algorithm>
#include <vector>
#include <SPI.h>


HDMIDisplay::HDMIDisplay(gpio_num_t cs)
{
  Serial.println("Starting up SPI");
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
  Serial.println("Adding SPI device for HDMI display");
  ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &devcfg, &spi));
  Serial.println("Allocating DMA buffer for HDMI display");
  dmaBuffer = (uint8_t *)heap_caps_malloc(6912 + 240 + 9, MALLOC_CAP_DMA);
}

bool HDMIDisplay::sendSpectrum(uint8_t *spectrumDisplay, uint8_t *borderColors)
{
  spi_device_acquire_bus(spi, portMAX_DELAY);
  spi_transaction_t transaction;
  memset(&transaction, 0, sizeof(transaction));
  // something freaky is going on with the SPI on the PI - it occasionally seems to be offset by 8 bytes
  // we'll fix this by adding on 8 bytes and a marker byte of 0xFF
  if (dmaBuffer == nullptr) {
    Serial.println("Failed to allocate DMA Buffer");
  }
  dmaBuffer[0] = 0xFF;
  memcpy(dmaBuffer + 1, spectrumDisplay, 6912);
  memcpy(dmaBuffer + 6912 + 1, borderColors + (312 - 240)/2, 240);
  transaction.length = (6912 + 240 + 9) * 8;
  transaction.tx_buffer = dmaBuffer;
  spi_device_polling_transmit(spi, &transaction);
  spi_device_release_bus(spi);
  return true;
}

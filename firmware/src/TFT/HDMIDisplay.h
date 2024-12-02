#pragma once
#include "Display.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "../Serial.h"

class HDMIDisplay
{
public:
  HDMIDisplay(gpio_num_t cs);
  bool sendSpectrum(uint8_t *spectrumDisplay, uint8_t *borderColors);
protected:
  uint8_t *dmaBuffer;
  spi_device_handle_t spi;
};
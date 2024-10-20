#pragma once

#include "driver/gpio.h"
#include "TFTDisplay.h"

class ILI9341: public TFTDisplay {
public:
    ILI9341(gpio_num_t mosi, gpio_num_t clk, gpio_num_t cs, gpio_num_t dc, gpio_num_t rst, gpio_num_t bl, int width = 240, int height = 320);
    ~ILI9341();
};

#pragma once

#include "driver/gpio.h"
#include "TFTDisplay.h"

class ST7789: public TFTDisplay {
public:
    ST7789(gpio_num_t cs, gpio_num_t dc, gpio_num_t rst, gpio_num_t bl, int width = 240, int height = 320);
    ~ST7789();
};

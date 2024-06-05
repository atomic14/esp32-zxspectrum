#pragma once

#include "Emulator/keyboard_defs.h"

class Nunchuck {
  private:
    unsigned int controller_type = 0;
    using KeyEventType = std::function<void(SpecKeys keyCode, bool isPressed)>;
    KeyEventType m_keyEvent;
  public:
  Nunchuck(KeyEventType keyEvent, gpio_num_t clk, gpio_num_t data);
  static void nunchuckTask(void *pParam);
};
#pragma once

#include "Emulator/keyboard_defs.h"

class Nunchuck {
  private:
    unsigned int controller_type = 0;
    using KeyEventType = std::function<void(SpecKeys keyCode, bool isPressed)>;
    KeyEventType m_keyEvent;
    using PressKeyEventType = std::function<void(SpecKeys keyCode)>;
    PressKeyEventType m_pressKeyEvent;
  public:
  Nunchuck(KeyEventType keyEvent, PressKeyEventType pressKeyEvent, gpio_num_t clk, gpio_num_t data);
  static void nunchuckTask(void *pParam);
};
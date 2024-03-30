#pragma once

class Nunchuck {
  private:
    unsigned int controller_type = 0;
    using KeyEventType = std::function<void(int keyCode, bool isPressed)>;
    KeyEventType m_keyEvent;
  public:
  Nunchuck(KeyEventType keyEvent, gpio_num_t clk, gpio_num_t data);
  static void nunchuckTask(void *pParam);
};
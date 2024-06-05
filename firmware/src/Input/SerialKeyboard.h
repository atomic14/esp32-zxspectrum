#pragma once

#include "../Emulator/keyboard_defs.h"

class SerialKeyboard {
  private:
    using KeyEventType = std::function<void(SpecKeys keyCode, bool isPressed)>;
    KeyEventType m_keyEvent;
  public:
  SerialKeyboard(KeyEventType keyEvent);
  static void keyboardTask(void *pParam);
};
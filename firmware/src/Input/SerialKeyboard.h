#pragma once

class SerialKeyboard {
  private:
    using KeyEventType = std::function<void(int keyCode, bool isPressed)>;
    KeyEventType m_keyEvent;
  public:
  SerialKeyboard(KeyEventType keyEvent);
  static void keyboardTask(void *pParam);
};
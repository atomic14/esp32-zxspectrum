#pragma once
#include <functional>
#include "../Emulator/keyboard_defs.h"

class TDeckKeyboard
{
private:
  // this is used for key down and key up events
  using KeyEventType = std::function<void(SpecKeys keyCode, bool isPressed)>;
  KeyEventType m_keyEvent;
  // this is used for key pressed evens and supports repeating keys
  using KeyPressedEventType = std::function<void(SpecKeys keyCode)>;
  KeyPressedEventType m_keyPressedEvent;
public:
  TDeckKeyboard(KeyEventType keyEvent, KeyPressedEventType keyPressedEvent);
  bool start();
  static void readKeyboardTask(void *pvParameters);
  void readKeyboard();
};

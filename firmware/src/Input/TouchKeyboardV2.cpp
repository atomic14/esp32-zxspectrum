#include "TouchKeyboardV2.h"

void TouchKeyboardV2::keyboardTask(void *arg)
{
#ifdef TOUCH_KEYBOARD_V2
  Serial.println("Touch Keyboard Task Started");
  TouchKeyboardV2 *touchKeyboard = (TouchKeyboardV2 *)arg;
  while (true)
  {
    for (int row = 0; row < 8; row++)
    {
      digitalWrite(14, (row & 1) ? HIGH : LOW);
      digitalWrite(21, (row & 2) ? HIGH : LOW);
      digitalWrite(47, (row & 4) ? HIGH : LOW);
      // delay for 5ms to let the touch FSM do it's magic
      vTaskDelay(5 / portTICK_PERIOD_MS);
      for (int touchPadIdx = 0; touchPadIdx < 5; touchPadIdx++)
      {
        SpecKeys keyCode = touchKeyboard->rows[row][touchPadIdx];
        uint32_t touch = 0;
        touch_pad_t pad = touchKeyboard->touchPads[touchPadIdx];
        touch_pad_read_raw_data(pad, &touch);
        if (touch > touchKeyboard->calibrations[row][touchPadIdx] * 1.1)
        {
          // touch detected
          // only send the key event if it's not already pressed
          if (touchKeyboard->isKeyPressed[keyCode] == false)
          {
            touchKeyboard->isKeyPressed[keyCode] = true;
            touchKeyboard->m_keyEvent(keyCode, true);
          }
        }
        else
        {
          // touch not detected
          // was the key previously pressed?
          if (touchKeyboard->isKeyPressed[keyCode] == true)
          {
            touchKeyboard->isKeyPressed[keyCode] = false;
            touchKeyboard->m_keyEvent(keyCode, false);
          }
        }
      }
    }
  }
#endif
}
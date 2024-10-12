#include "TouchKeyboardV2.h"

void TouchKeyboardV2::keyboardTask(void *arg)
{
#ifdef TOUCH_KEYBOARD_V2
  Serial.println("Touch Keyboard Task Started");
  TouchKeyboardV2 *touchKeyboard = (TouchKeyboardV2 *)arg;
  while (true)
  {
    unsigned long timeNow = millis();
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
          if (touchKeyboard->isKeyPressed[keyCode] == 0)
          {
            // the repease time for the key
            touchKeyboard->isKeyPressed[keyCode] = timeNow;
            // the key is down
            touchKeyboard->m_keyEvent(keyCode, true);
            // initial key press
            touchKeyboard->m_keyPressedEvent(keyCode);
          }
          else
          {
            // key is being held down - should we repeat it?
            unsigned long timeSinceLastPress = timeNow - touchKeyboard->isKeyPressed[keyCode];
            if (timeSinceLastPress > 500)
            {
              // are we doing fast repeat? e.g. the key has been held down for more than 500ms
              // we'll repeat the key every 100ms. To detect this we'll set the last press time to now - 400ms
              // that way, next time we check, we'll repeat the key
              touchKeyboard->isKeyPressed[keyCode] = timeNow - 400;
              touchKeyboard->m_keyPressedEvent(keyCode);
            }
          }
        }
        else
        {
          // touch not detected
          // was the key previously pressed?
          if (touchKeyboard->isKeyPressed[keyCode] != 0)
          {
            touchKeyboard->isKeyPressed[keyCode] = 0;
            touchKeyboard->m_keyEvent(keyCode, false);
          }
        }
      }
    }
  }
#endif
}
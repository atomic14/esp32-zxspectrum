#include "Wire.h"
#include "../Serial.h"
#include "TDeckKeyboard.h"

TDeckKeyboard::TDeckKeyboard(KeyEventType keyEvent, KeyPressedEventType keyPressedEvent) : m_keyEvent(keyEvent), m_keyPressedEvent(keyPressedEvent)
{
}

bool TDeckKeyboard::start() {
  Wire.begin(GPIO_NUM_18, GPIO_NUM_8);
  Wire.requestFrom(0x55, 1);
  if (Wire.read() == -1) {
    Serial.println("LILYGO Keyboad not online .");
    return false;
  }
  // kick off a task to read the keyboard
  xTaskCreate(readKeyboardTask, "readKeyboardTask", 4096, this, 5, NULL);
  return true;
}

void TDeckKeyboard::readKeyboardTask(void *pvParameters) {
  TDeckKeyboard *self = (TDeckKeyboard *)pvParameters;
  self->readKeyboard();
}

void TDeckKeyboard::readKeyboard() {
  SpecKeys lastKey = SpecKeys::SPECKEY_NONE;
  unsigned long lastKeyPress = 0;
  while(true) {
    char keyValue = 0;
    Wire.requestFrom(0x55, 1);
    while (Wire.available() > 0) {
        keyValue = Wire.read();
        if (keyValue != (char)0x00) {
            // is it a valid spec key?
            // convert to uppercase
            if (letterToSpecKey.find(keyValue) != letterToSpecKey.end()) {
              SpecKeys key = letterToSpecKey.at(keyValue);
              m_keyPressedEvent(key);
              m_keyEvent(key, true);
              lastKey = key;
              lastKeyPress = millis();
            }
        }
        // release any pressed keys
        if (lastKey != SpecKeys::SPECKEY_NONE && millis() - lastKeyPress > 100) {
          m_keyEvent(lastKey, false);
          lastKey = SpecKeys::SPECKEY_NONE;
        }
    }
    vTaskDelay(10);
  }
}
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
  std::vector<SpecKeys> keysPressed;
  unsigned long lastPress = 0;
  while(true) {
    char keyValue = 0;
    Wire.requestFrom(0x55, 1);
    while (Wire.available() > 0) {
        keyValue = Wire.read();
        if (keyValue != (char)0x00) {
            Serial.println(keyValue);
            // is it a valid spec key?
            // convert to uppercase
            if (letterToSpecKeys.find(keyValue) != letterToSpecKeys.end()) {
              std::vector<SpecKeys> keys = letterToSpecKeys.at(keyValue);
              for (SpecKeys key : keys) {
                keysPressed.push_back(key);
                m_keyPressedEvent(key);
                m_keyEvent(key, true);
                lastPress = millis();
              }
            }
        }
        // release any pressed keys
        if (millis() - lastPress > 100) {
          for (SpecKeys key : keysPressed) {
            m_keyEvent(key, false);
          }
          keysPressed.clear();
        }
    }
    vTaskDelay(10);
  }
}
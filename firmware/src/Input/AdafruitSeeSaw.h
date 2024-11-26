#pragma once

#include <Arduino.h>
#include <Adafruit_seesaw.h>
#include "../Serial.h"
#include "../Emulator/spectrum.h"

#define BUTTON_X 6
#define BUTTON_Y 2
#define BUTTON_A 5
#define BUTTON_B 1
#define BUTTON_SELECT 0
#define BUTTON_START 16
uint32_t button_mask = (1UL << BUTTON_X) | (1UL << BUTTON_Y) | (1UL << BUTTON_START) |
                       (1UL << BUTTON_A) | (1UL << BUTTON_B) | (1UL << BUTTON_SELECT);

class AdafruitSeeSaw
{
private:
  Adafruit_seesaw *ss;
  using KeyEventType = std::function<void(SpecKeys keyCode, bool isPressed)>;
  KeyEventType m_keyEvent;
  using PressKeyEventType = std::function<void(SpecKeys keyCode)>;
  PressKeyEventType m_pressKeyEvent;

public:
  AdafruitSeeSaw(KeyEventType keyEvent, PressKeyEventType pressKeyEvent) : m_keyEvent(keyEvent), m_pressKeyEvent(pressKeyEvent)
  {
  }

  bool begin(gpio_num_t data, gpio_num_t clk)
  {
    TwoWire *i2c = new TwoWire(0);
    i2c->setPins(GPIO_NUM_44, GPIO_NUM_43);
    ss = new Adafruit_seesaw(i2c);
    if (!ss->begin(0x50))
    {
      delete ss;
      delete i2c;
      Serial.printf("ERROR initializing seesaw controller\n");
      return false;
    }
    Serial.println("seesaw started");
    uint32_t version = ((ss->getVersion() >> 16) & 0xFFFF);
    if (version != 5743)
    {
      Serial.print("Wrong firmware loaded? ");
      Serial.println(version);
    }
    Serial.println("Found Product 5743");
    ss->pinModeBulk(button_mask, INPUT_PULLUP);
    // TODO - do we need this?
    // ss->setGPIOInterrupts(button_mask, 1);
    xTaskCreatePinnedToCore(seeSawTask, "seeSawTask", 4096, this, 1, NULL, 0);
    return true;
  }

  static void seeSawTask(void *pParam)
  {
    AdafruitSeeSaw *seesaw = (AdafruitSeeSaw *)pParam;
    std::unordered_map<SpecKeys, int> keyStates = {
        {JOYK_UP, 0},
        {JOYK_DOWN, 0},
        {JOYK_LEFT, 0},
        {JOYK_RIGHT, 0},
        {JOYK_FIRE, 0}};

    while (true)
    {
      vTaskDelay(10 / portTICK_PERIOD_MS);
      std::vector<SpecKeys> detectedKeys;
      int x = 1023 - seesaw->ss->analogRead(14);
      int y = 1023 - seesaw->ss->analogRead(15);
      if (x > 1024 - 300)
      { // going right
        detectedKeys.push_back(JOYK_RIGHT);
      }
      else if (x < 300)
      { // going left
        detectedKeys.push_back(JOYK_LEFT);
      }
      if (y > 1024 - 300)
      { // going down
        detectedKeys.push_back(JOYK_UP);
      }
      else if (y < 300)
      { // going up
        detectedKeys.push_back(JOYK_DOWN);
      }

      uint32_t buttons = seesaw->ss->digitalReadBulk(button_mask);
      if (!(buttons & (1UL << BUTTON_A)))
      {
        detectedKeys.push_back(JOYK_FIRE);
      }
      // TODO - wire these buttons up
      // if (! (buttons & (1UL << BUTTON_B))) {
      //   Serial.println("Button B pressed");
      // }
      // if (! (buttons & (1UL << BUTTON_Y))) {
      //   Serial.println("Button Y pressed");
      // }
      // if (! (buttons & (1UL << BUTTON_X))) {
      //   Serial.println("Button X pressed");
      // }
      // if (! (buttons & (1UL << BUTTON_SELECT))) {
      //   Serial.println("Button SELECT pressed");
      // }
      // if (! (buttons & (1UL << BUTTON_START))) {
      //   Serial.println("Button START pressed");
      // }
      // unpress any keys that are not detected
      for (auto &keyState : keyStates)
      {
        if (std::find(detectedKeys.begin(), detectedKeys.end(), keyState.first) == detectedKeys.end())
        {
          if (keyState.second != 0)
          {
            seesaw->m_keyEvent(keyState.first, false);
            keyState.second = 0;
          }
        }
      }
      // for each detected key - check if it's a new press and fire off any events
      int now = millis();
      for (SpecKeys key : detectedKeys)
      {
        // send the first event if the key has not been pressed yet
        if (keyStates[key] == 0)
        {
          seesaw->m_keyEvent(key, true);
          seesaw->m_pressKeyEvent(key);
          keyStates[key] = now;
        }
        else if (now - keyStates[key] > 500)
        {
          // we need to repeat the key
          seesaw->m_pressKeyEvent(key);
          // we'll repeat after 100 ms from now on
          keyStates[key] = now - 400;
        }
      }
    }
  }
};
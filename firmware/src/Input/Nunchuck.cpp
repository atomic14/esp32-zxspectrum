#include <Arduino.h>
#include "../Serial.h"
#include "../Emulator/spectrum.h"
#include "wii_i2c/wii_i2c.h"
#include "Nunchuck.h"

Nunchuck::Nunchuck(KeyEventType keyEvent, PressKeyEventType pressKeyEvent, gpio_num_t clk, gpio_num_t data) : m_keyEvent(keyEvent), m_pressKeyEvent(pressKeyEvent)
{
  if (wii_i2c_init(0, data, clk) != 0)
  {
    Serial.printf("ERROR initializing wii i2c controller\n");
  }
  else
  {
    const unsigned char *ident = wii_i2c_read_ident();
    if (!ident)
    {
      Serial.printf("no ident :(\n");
    }
    else
    {
      controller_type = wii_i2c_decode_ident(ident);
      switch (controller_type)
      {
      case WII_I2C_IDENT_NUNCHUK:
        Serial.printf("-> nunchuk detected\n");
        break;
      case WII_I2C_IDENT_CLASSIC:
        Serial.printf("-> classic controller detected\n");
        break;
      default:
        Serial.printf("-> unknown controller detected: 0x%06x\n", controller_type);
        break;
      }
    }
  }
  xTaskCreatePinnedToCore(nunchuckTask, "nunchuckTask", 4096, this, 1, NULL, 0);
}

void Nunchuck::nunchuckTask(void *pParam)
{
  Nunchuck *nunchuck = (Nunchuck *)pParam;
  std::unordered_map<SpecKeys, int> keyStates = {
      {JOYK_UP, 0},
      {JOYK_DOWN, 0},
      {JOYK_LEFT, 0},
      {JOYK_RIGHT, 0},
      {JOYK_FIRE, 0}};

  while (true)
  {
    wii_i2c_request_state();
    vTaskDelay(10 / portTICK_PERIOD_MS);
    const unsigned char *data = wii_i2c_read_state();
    if (data)
    {
      if (nunchuck->controller_type == WII_I2C_IDENT_NUNCHUK)
      {
        std::vector<SpecKeys> detectedKeys;
        wii_i2c_nunchuk_state state;
        wii_i2c_decode_nunchuk(data, &state);
        if (state.x > 50)
        { // going right
          detectedKeys.push_back(JOYK_RIGHT);
        }
        else if (state.x < -50)
        { // going left
          detectedKeys.push_back(JOYK_LEFT);
        }
        if (state.y > 50)
        { // going up
          detectedKeys.push_back(JOYK_UP);
        }
        else if (state.y < -50)
        { // going down
          detectedKeys.push_back(JOYK_DOWN);
        }
        if (state.z || state.c)
        {
          detectedKeys.push_back(JOYK_FIRE);
        }
        // unpress any keys that are not detected
        for (auto &keyState : keyStates)
        {
          if (std::find(detectedKeys.begin(), detectedKeys.end(), keyState.first) == detectedKeys.end())
          {
            if (keyState.second != 0)
            {
              nunchuck->m_keyEvent(keyState.first, false);
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
            nunchuck->m_keyEvent(key, true);
            nunchuck->m_pressKeyEvent(key);
            keyStates[key] = now;
          }
          else if (now - keyStates[key] > 500)
          {
            // we need to repeat the key
            nunchuck->m_pressKeyEvent(key);
            // we'll repeat after 100 ms from now on
            keyStates[key] = now - 400;
          }
        }
      }
    }
  }
}
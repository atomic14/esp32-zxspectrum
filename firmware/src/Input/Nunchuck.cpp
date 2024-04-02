#include <Arduino.h>
#include "../Emulator/spectrum.h"
#include "wii_i2c/wii_i2c.h"
#include "Nunchuck.h"

Nunchuck::Nunchuck(KeyEventType keyEvent, gpio_num_t clk, gpio_num_t data) : m_keyEvent(keyEvent)
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

void Nunchuck::nunchuckTask(void *pParam) {
  Nunchuck *nunchuck = (Nunchuck *)pParam;
  bool wasUp = false;
  bool wasDown = false;
  bool wasLeft = false;
  bool wasRight = false;
  bool wasFire = false;
  while(true) {
      wii_i2c_request_state();
      vTaskDelay(10/ portTICK_PERIOD_MS);
      const unsigned char *data = wii_i2c_read_state();
      if (data)
      {
        switch (nunchuck->controller_type)
        {
        case WII_I2C_IDENT_NUNCHUK:
        {
          wii_i2c_nunchuk_state state;
          wii_i2c_decode_nunchuk(data, &state);
          if (state.x > 50)
          { // going right
            nunchuck->m_keyEvent(JOYK_RIGHT, 1);
            wasRight = true;
          }
          else if (state.x < -50)
          { // going left
            nunchuck->m_keyEvent(JOYK_LEFT, 1);
            wasLeft = true;
          }
          else
          {
            if (wasLeft) {
              nunchuck->m_keyEvent(JOYK_LEFT, 0);
              wasLeft = false;
            }
            if (wasRight) {
              nunchuck->m_keyEvent(JOYK_RIGHT, 0);
              wasRight = false;
            }
          }
          if (state.y > 50)
          { // going up
            nunchuck->m_keyEvent(JOYK_UP, 1);
            wasUp = true;
          }
          else if (state.y < -50)
          { // going down
            nunchuck->m_keyEvent(JOYK_DOWN, 1);
            wasDown = true;
          }
          else
          {
            if (wasUp) {
              nunchuck->m_keyEvent(JOYK_UP, 0);
              wasUp = false;
            }
            if (wasDown) {
              nunchuck->m_keyEvent(JOYK_DOWN, 0);
              wasDown = false;
            }
          }
          if (state.z || state.c)
          {
            nunchuck->m_keyEvent(JOYK_FIRE, 1);
            wasFire = true;
          }
          else
          {
            if (wasFire) {
              nunchuck->m_keyEvent(JOYK_FIRE, 0);
              wasFire = false;
            }
          }
        }
        break;
        }
    }
  }
}
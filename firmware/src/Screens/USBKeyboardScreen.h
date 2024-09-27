#pragma once

#include "USB.h"
#include "USBHIDKeyboard.h"
#include <vector>
#include <string>
#include "Screen.h"
#include "../TFT/TFTDisplay.h"
#include "../Emulator/spectrum.h"

class TFTDisplay;

#ifndef ARDUINO_USB_MODE
#error This ESP32 SoC has no Native USB interface
#elif ARDUINO_USB_MODE == 1
#warning This sketch should be used when USB is in OTG mode
#endif

const std::unordered_map<SpecKeys, int> specKeyToHidKey = {
    {SPECKEY_SHIFT, HID_KEY_SHIFT_LEFT},
    {SPECKEY_ENTER, HID_KEY_RETURN},
    {SPECKEY_SPACE, HID_KEY_SPACE},
    {SPECKEY_SYMB, HID_KEY_ALT_RIGHT},
    {SPECKEY_1, HID_KEY_1},
    {SPECKEY_2, HID_KEY_2},
    {SPECKEY_3, HID_KEY_3},
    {SPECKEY_4, HID_KEY_4},
    {SPECKEY_5, HID_KEY_5},
    {SPECKEY_6, HID_KEY_6},
    {SPECKEY_7, HID_KEY_7},
    {SPECKEY_8, HID_KEY_8},
    {SPECKEY_9, HID_KEY_9},
    {SPECKEY_0, HID_KEY_0},
    {SPECKEY_A, HID_KEY_A},
    {SPECKEY_B, HID_KEY_B},
    {SPECKEY_C, HID_KEY_C},
    {SPECKEY_D, HID_KEY_D},
    {SPECKEY_E, HID_KEY_E},
    {SPECKEY_F, HID_KEY_F},
    {SPECKEY_G, HID_KEY_G},
    {SPECKEY_H, HID_KEY_H},
    {SPECKEY_I, HID_KEY_I},
    {SPECKEY_J, HID_KEY_J},
    {SPECKEY_K, HID_KEY_K},
    {SPECKEY_L, HID_KEY_L},
    {SPECKEY_M, HID_KEY_M},
    {SPECKEY_N, HID_KEY_N},
    {SPECKEY_O, HID_KEY_O},
    {SPECKEY_P, HID_KEY_P},
    {SPECKEY_Q, HID_KEY_Q},
    {SPECKEY_R, HID_KEY_R},
    {SPECKEY_S, HID_KEY_S},
    {SPECKEY_T, HID_KEY_T},
    {SPECKEY_U, HID_KEY_U},
    {SPECKEY_V, HID_KEY_V},
    {SPECKEY_W, HID_KEY_W},
    {SPECKEY_X, HID_KEY_X},
    {SPECKEY_Y, HID_KEY_Y},
    {SPECKEY_Z, HID_KEY_Z},
};

class USBKeyboardScreen : public Screen
{
private:
  USBHIDKeyboard keyboard;
public:
  USBKeyboardScreen(
      TFTDisplay &tft,
      AudioOutput *audioOutput) : Screen(tft, audioOutput)
  {
  }

  void didAppear()
  {
    updateDisplay();
    keyboard.begin();
    USB.begin();
  }
  
  void updatekey(SpecKeys key, uint8_t state)
  {
    // map from the spectrum key to a hid_key
    int hid_key = 0;
    if (specKeyToHidKey.find(key) != specKeyToHidKey.end())
    {
      hid_key = specKeyToHidKey.at(key);
    } else {
      return;
    }

    if (state == 1)
    {
      // key is pressed
      keyboard.pressRaw(hid_key);
    }
    else
    {
      // key is released
      keyboard.releaseRaw(hid_key);
    }
  }

  void updateDisplay()
  {
    m_tft.startWrite();
    m_tft.fillScreen(TFT_BLACK);
    m_tft.setTextColor(TFT_WHITE, TFT_BLACK);
    m_tft.drawString("USB Keyboard", 20, 10);
    m_tft.endWrite();
  }
};

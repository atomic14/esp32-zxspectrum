#pragma once

#include <Arduino.h>
#include <unordered_map>
#include <vector>
#include <string>
#include "../Emulator/spectrum.h"

const int TOUCH_CALIBRATION_SAMPLES = 100;
const float TOUCH_THRESHOLD = 0.1;

class TouchPad
{
private:
  using TouchChangeEvent = std::function<void()>;
  TouchChangeEvent mTouchEvent;

  int mPin = -1;
  std::string mName;
  // special handling for symbol shift and caps shift keys - our keyboard is not quite right so 
  // we can't detect CAPS and SYMB keys at the same time as some other keys
  // TODO - is this just some weirdness with touch not working properly?
  bool isToggle = false;
  bool mIsTouched = false;
  unsigned long mThreshold = 0;

public:
  TouchPad(int pin, std::string name, TouchChangeEvent touchEvent) : mPin(pin), mName(name), mTouchEvent(touchEvent)
  {
  }
  void calibrate()
  {
    // assume the pin is not touched and calculate the median value for untouched state
    unsigned long samples[TOUCH_CALIBRATION_SAMPLES];
    // touchSetCycles(10000, 1000);
    for (int i = 0; i < TOUCH_CALIBRATION_SAMPLES; i++)
    {
      samples[i] = touchRead(mPin);
      delay(1);
    }
    std::sort(samples, samples + TOUCH_CALIBRATION_SAMPLES);
    // the median value is the value at the midpoint of the sorted array
    mThreshold = (1 + TOUCH_THRESHOLD) * samples[TOUCH_CALIBRATION_SAMPLES / 2];
    Serial.printf("Calibrated %s(%d) to %d\n", mName.c_str(), mPin, mThreshold);
  }
  void start()
  {
    // Ideally we'd like to use the touchAttachInterrupt family of functions, but this can only
    // be used to detect that a touch has happened. There doesn't seem to be any way to detect when
    // the touch has finished. So instead we'll have a task that polls the touch pin using touchRead
    // and sets a flag while the pin is being touched.
    // TODO - investigate the native ESP32 touch driver for a better solution
    xTaskCreatePinnedToCore(
        [](void *arg)
        {
          TouchPad *pad = (TouchPad *)arg;
          for (;;)
          {
            unsigned long value = touchRead(pad->mPin);
            if (value > pad->mThreshold) {
              if (!pad->mIsTouched) {
                pad->mIsTouched = true;
                pad->mTouchEvent();  
              }
            } else {
              if (pad->mIsTouched && !pad->isToggle) {
                pad->mIsTouched = false;
                pad->mTouchEvent();
              }
            }
            vTaskDelay(10 / portTICK_PERIOD_MS);
          }
        },
        "touchTask",
        4096,
        this,
        1,
        NULL,
        0);
  }
  bool isTouched()
  {
    return mIsTouched;
  }
};

class TouchKeyboard
{
private:
  SemaphoreHandle_t m_keyboardSemaphore;

  using KeyEventType = std::function<void(int keyCode, bool isPressed)>;
  KeyEventType m_keyEvent;
  SpecKeys lastKeyPressed = SPECKEY_NONE;

public:
  TouchPad padBit0;
  TouchPad padBit1;
  TouchPad padBit2;
  TouchPad padBit3;
  TouchPad padBit4;
  TouchPad padFEFE;
  TouchPad padFDFE;
  TouchPad padFBFE;
  TouchPad padF7FE;
  TouchPad padEFFE;
  TouchPad padDFFE;
  TouchPad padBFFE;
  TouchPad pad7FFE;

  std::vector<TouchPad *> ports = {
    &padFEFE,
    &padFDFE,
    &padFBFE,
    &padF7FE,
    &padEFFE,
    &padDFFE,
    &padBFFE,
    &pad7FFE
  };

  std::vector<TouchPad *> bits = {
    &padBit0,
    &padBit1,
    &padBit2,
    &padBit3,
    &padBit4
  };

  // Mapping of port and bit index to keys
  std::unordered_map<int, std::vector<SpecKeys>> keyMap = {
    {0, {SPECKEY_0, SPECKEY_Z, SPECKEY_X, SPECKEY_C, SPECKEY_V}},
    {1, {SPECKEY_A, SPECKEY_S, SPECKEY_D, SPECKEY_F, SPECKEY_G}},
    {2, {SPECKEY_Q, SPECKEY_W, SPECKEY_E, SPECKEY_R, SPECKEY_T}},
    {3, {SPECKEY_1, SPECKEY_2, SPECKEY_3, SPECKEY_4, SPECKEY_5}},
    {4, {SPECKEY_0, SPECKEY_9, SPECKEY_8, SPECKEY_7, SPECKEY_6}},
    {5, {SPECKEY_P, SPECKEY_O, SPECKEY_I, SPECKEY_U, SPECKEY_Y}},
    {6, {SPECKEY_ENTER, SPECKEY_L, SPECKEY_K, SPECKEY_J, SPECKEY_H}},
    {7, {SPECKEY_SPACE, SPECKEY_SYMB, SPECKEY_M, SPECKEY_N, SPECKEY_B}}
  };

  TouchKeyboard(KeyEventType keyEvent) : 
    m_keyEvent(keyEvent),
    padBit0(12, "bit0", [this](){sendKeyEvent();}),
    padBit1(11, "bit1", [this](){sendKeyEvent();}),
    padBit2(10, "bit2", [this](){sendKeyEvent();}),
    padBit3(3, "bit3", [this](){sendKeyEvent();}),
    padBit4(8, "bit4", [this](){sendKeyEvent();}),
    padFEFE(7, "FEFE", [this](){sendKeyEvent();}),
    padFDFE(6, "FDFE", [this](){sendKeyEvent();}),
    padFBFE(5, "FBFE", [this](){sendKeyEvent();}),
    padF7FE(4, "F7FE", [this](){sendKeyEvent();}),
    padEFFE(13, "EFFE", [this](){sendKeyEvent();}),
    padDFFE(9, "DFFE", [this](){sendKeyEvent();}),
    padBFFE(2, "BFFE", [this](){sendKeyEvent();}),
    pad7FFE(1, "7FFE", [this](){sendKeyEvent();})
  {
    m_keyboardSemaphore = xSemaphoreCreateBinary();
    xSemaphoreGive(m_keyboardSemaphore);
  };
  void calibrate()
  {
    padBit0.calibrate();
    padBit1.calibrate();
    padBit2.calibrate();
    padBit3.calibrate();
    padBit4.calibrate();
    padFEFE.calibrate();
    padFDFE.calibrate();
    padFBFE.calibrate();
    padF7FE.calibrate();
    padEFFE.calibrate();
    padDFFE.calibrate();
    padBFFE.calibrate();
    pad7FFE.calibrate();
  }
  void start() {
    padBit0.start();
    padBit1.start();
    padBit2.start();
    padBit3.start();
    padBit4.start();
    padFEFE.start();
    padFDFE.start();
    padFBFE.start();
    padF7FE.start();
    padEFFE.start();
    padDFFE.start();
    padBFFE.start();
    pad7FFE.start();
  }

  SpecKeys getCurrentKey() {
    // work out which key is pressed (or if no keys are pressed...)
    for (int portIndex = 0; portIndex < ports.size(); ++portIndex) {
        if (ports[portIndex]->isTouched()) {
            for (int bitIndex = 0; bitIndex < bits.size(); ++bitIndex) {
                if (bits[bitIndex]->isTouched()) {
                    return keyMap[portIndex][bitIndex];
                }
            }
        }
    }
    return SPECKEY_NONE;
  }

  void sendKeyEvent() {
    if (xSemaphoreTake(m_keyboardSemaphore, portMAX_DELAY)) {
      SpecKeys key = getCurrentKey();
      if (key != lastKeyPressed) {
        if (lastKeyPressed != SPECKEY_NONE) {
          m_keyEvent(lastKeyPressed, false);
          Serial.printf("Key up: %d\n", lastKeyPressed);
        }
      }
      if (key != SPECKEY_NONE) {
        m_keyEvent(key, true);
        Serial.printf("Key down: %d\n", key);
      }
      lastKeyPressed = key;
    }
    xSemaphoreGive(m_keyboardSemaphore);
  }
};
#pragma once

#include <Arduino.h>
#include <unordered_map>
#include <set>
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
            if (value > pad->mThreshold)
            {
              if (!pad->mIsTouched)
              {
                pad->mIsTouched = true;
                pad->mTouchEvent();
              }
            }
            else
            {
              if (pad->mIsTouched && !pad->isToggle)
              {
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
      &pad7FFE};

  std::vector<TouchPad *> bits = {
      &padBit0,
      &padBit1,
      &padBit2,
      &padBit3,
      &padBit4};

  // Mapping of port and bit index to keys
  std::unordered_map<int, std::vector<SpecKeys>> keyMap = {
      {0, {SPECKEY_SHIFT, SPECKEY_Z, SPECKEY_X, SPECKEY_C, SPECKEY_V}},
      {1, {SPECKEY_A, SPECKEY_S, SPECKEY_D, SPECKEY_F, SPECKEY_G}},
      {2, {SPECKEY_Q, SPECKEY_W, SPECKEY_E, SPECKEY_R, SPECKEY_T}},
      {3, {SPECKEY_1, SPECKEY_2, SPECKEY_3, SPECKEY_4, SPECKEY_5}},
      {4, {SPECKEY_0, SPECKEY_9, SPECKEY_8, SPECKEY_7, SPECKEY_6}},
      {5, {SPECKEY_P, SPECKEY_O, SPECKEY_I, SPECKEY_U, SPECKEY_Y}},
      {6, {SPECKEY_ENTER, SPECKEY_L, SPECKEY_K, SPECKEY_J, SPECKEY_H}},
      {7, {SPECKEY_SPACE, SPECKEY_SYMB, SPECKEY_M, SPECKEY_N, SPECKEY_B}}};

  TouchKeyboard(KeyEventType keyEvent) : m_keyEvent(keyEvent),
                                         padBit0(12, "bit0", [this]()
                                                 { sendKeyEvent(); }),
                                         padBit1(11, "bit1", [this]()
                                                 { sendKeyEvent(); }),
                                         padBit2(10, "bit2", [this]()
                                                 { sendKeyEvent(); }),
                                         padBit3(3, "bit3", [this]()
                                                 { sendKeyEvent(); }),
                                         padBit4(8, "bit4", [this]()
                                                 { sendKeyEvent(); }),
                                         padFEFE(7, "FEFE", [this]()
                                                 { sendKeyEvent(); }),
                                         padFDFE(6, "FDFE", [this]()
                                                 { sendKeyEvent(); }),
                                         padFBFE(5, "FBFE", [this]()
                                                 { sendKeyEvent(); }),
                                         padF7FE(4, "F7FE", [this]()
                                                 { sendKeyEvent(); }),
                                         padEFFE(13, "EFFE", [this]()
                                                 { sendKeyEvent(); }),
                                         padDFFE(9, "DFFE", [this]()
                                                 { sendKeyEvent(); }),
                                         padBFFE(2, "BFFE", [this]()
                                                 { sendKeyEvent(); }),
                                         pad7FFE(1, "7FFE", [this]()
                                                 { sendKeyEvent(); })
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
  void start()
  {
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

  std::unordered_map<SpecKeys, bool> isKeyPressed = {
      {SPECKEY_1, false},
      {SPECKEY_2, false},
      {SPECKEY_3, false},
      {SPECKEY_4, false},
      {SPECKEY_5, false},
      {SPECKEY_6, false},
      {SPECKEY_7, false},
      {SPECKEY_8, false},
      {SPECKEY_9, false},
      {SPECKEY_0, false},
      {SPECKEY_Q, false},
      {SPECKEY_W, false},
      {SPECKEY_E, false},
      {SPECKEY_R, false},
      {SPECKEY_T, false},
      {SPECKEY_Y, false},
      {SPECKEY_U, false},
      {SPECKEY_I, false},
      {SPECKEY_O, false},
      {SPECKEY_P, false},
      {SPECKEY_A, false},
      {SPECKEY_S, false},
      {SPECKEY_D, false},
      {SPECKEY_F, false},
      {SPECKEY_G, false},
      {SPECKEY_H, false},
      {SPECKEY_J, false},
      {SPECKEY_K, false},
      {SPECKEY_L, false},
      {SPECKEY_ENTER, false},
      {SPECKEY_SHIFT, false},
      {SPECKEY_Z, false},
      {SPECKEY_X, false},
      {SPECKEY_C, false},
      {SPECKEY_V, false},
      {SPECKEY_B, false},
      {SPECKEY_N, false},
      {SPECKEY_M, false},
      {SPECKEY_SYMB, false},
      {SPECKEY_SPACE, false}
    };

  std::unordered_map<SpecKeys, bool> toggleKey = {
      {SPECKEY_SHIFT, false},
      {SPECKEY_SYMB, false}
  };

  void sendKeyEvent()
  {
    if (xSemaphoreTake(m_keyboardSemaphore, portMAX_DELAY))
    {
      // work out which key is pressed (or if no keys are pressed...)
      for (int portIndex = 0; portIndex < ports.size(); ++portIndex)
      {
        for (int bitIndex = 0; bitIndex < bits.size(); ++bitIndex)
        {
          // this is the key that we are examining
          SpecKeys theKey = keyMap[portIndex][bitIndex];
          // is it pressed?
          if (bits[bitIndex]->isTouched() && ports[portIndex]->isTouched())
          {
            // if it was already pressed then don't do anything
            if (!isKeyPressed[theKey])
            {
              // otherwise we need to press it
              m_keyEvent(theKey, true);
              isKeyPressed[theKey] = true;
            }
          } else {
            // if the key was pressed then we need to release it
            if (isKeyPressed[theKey])
            {
              // special handling for symbol shift if the caps shift key is already toggled - we want to release the sym shift and caps shift keys
              if (theKey == SPECKEY_SYMB && toggleKey[SPECKEY_SHIFT])
              {
                m_keyEvent(SPECKEY_SHIFT, false);
                m_keyEvent(SPECKEY_SYMB, false);
                toggleKey[SPECKEY_SHIFT] = false;
              } else {
                // check to see if this is a toggle key - ie, it's in the toggleKey map
                if (toggleKey.find(theKey) != toggleKey.end())
                {
                  // do we need to release the key?
                  if (toggleKey[theKey]) {
                    m_keyEvent(theKey, false);
                    toggleKey[theKey] = false;
                  } else {
                    // toggle it the next time round
                    toggleKey[theKey] = true;
                  }
                } else {
                  // otherwise we need to release the key
                  m_keyEvent(theKey, false);
                  // and if there are any toggle keys then we need to release them too
                  for (auto &toggle : toggleKey)
                  {
                    if (toggle.second)
                    {
                      m_keyEvent(toggle.first, false);
                      toggle.second = false;
                    }
                  }
                }
              }
              isKeyPressed[theKey] = false;
            }
          }
        }
      }
    }
    xSemaphoreGive(m_keyboardSemaphore);
  }
};
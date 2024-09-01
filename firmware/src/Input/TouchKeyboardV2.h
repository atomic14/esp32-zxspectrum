#pragma once

#include <Arduino.h>
#include "driver/touch_pad.h"
#include <unordered_map>
#include <set>
#include <vector>
#include <string>
#include "../Emulator/keyboard_defs.h"

const int TOUCH_CALIBRATION_SAMPLES = 100;
const float TOUCH_THRESHOLD = 0.1;

class TouchKeyboardV2
{
private:
  using KeyEventType = std::function<void(SpecKeys keyCode, bool isPressed)>;
  KeyEventType m_keyEvent;

  // touch pad calibration values
  uint32_t calibrations[8][5] = {0};
#ifdef TOUCH_KEYBOARD_V2
  // all the touch pads we are using
  touch_pad_t touchPads[5] = {TOUCH_PAD_NUM13, TOUCH_PAD_NUM12, TOUCH_PAD_NUM11, TOUCH_PAD_NUM10, TOUCH_PAD_NUM9};
#endif

  void setupTouchPad()
  {
#ifdef TOUCH_KEYBOARD_V2
    touch_pad_init();
    // this is the magic line to make it go faster... no real idea what these values really do...
    touch_pad_set_meas_time(4, 50);
    touch_pad_set_voltage(TOUCH_PAD_HIGH_VOLTAGE_THRESHOLD, TOUCH_PAD_LOW_VOLTAGE_THRESHOLD, TOUCH_PAD_ATTEN_VOLTAGE_THRESHOLD);
    touch_pad_set_idle_channel_connect(TOUCH_PAD_IDLE_CH_CONNECT_DEFAULT);
    touch_pad_denoise_t denoise = {
        .grade = TOUCH_PAD_DENOISE_BIT4,
        .cap_level = TOUCH_PAD_DENOISE_CAP_L4,
    };
    touch_pad_denoise_set_config(&denoise);
    touch_pad_denoise_enable();
    // Touch Sensor Timer initiated
    touch_pad_set_fsm_mode(TOUCH_FSM_MODE_TIMER); // returns ESP_OK
    touch_pad_fsm_start();
    for (int touchPadIdx = 0; touchPadIdx < 5; touchPadIdx++)
    {
      touch_pad_config(touch_pad_t(touchPads[touchPadIdx]));
    }
#endif
  }

  void calibrate()
  {
#ifdef TOUCH_KEYBOARD_V2
    for (int i = 0; i < 8; i++)
    {
      digitalWrite(14, (i & 1) ? HIGH : LOW);
      digitalWrite(21, (i & 2) ? HIGH : LOW);
      digitalWrite(47, (i & 4) ? HIGH : LOW);
      delay(10);
      // calibration - take the average of 5 readings
      for (int touchPadIdx = 0; touchPadIdx < 5; touchPadIdx++)
      {
        unsigned long aveTouch = 0;
        for (int j = 0; j < 5; j++)
        {
          uint32_t touch;
          touch_pad_read_raw_data(touch_pad_t(touchPads[touchPadIdx]), &touch);
          aveTouch += touch;
        }
        calibrations[i][touchPadIdx] = aveTouch / 5;
        Serial.printf("Calibration %d,%d: %d\n", i, touchPadIdx, calibrations[i][touchPadIdx]);
      }
    }
#endif
  }

  static void keyboardTask(void *arg);

public:
  // mapping from address to bit to spectrum key
  // we have 8 addresses and 5 bits per address
  SpecKeys rows[8][5] = {
      // FEFE
      {SPECKEY_SHIFT, SPECKEY_Z, SPECKEY_X, SPECKEY_C, SPECKEY_V},
      // FBFE
      {SPECKEY_Q, SPECKEY_W, SPECKEY_E, SPECKEY_R, SPECKEY_T},
      // F7FE
      {SPECKEY_1, SPECKEY_2, SPECKEY_3, SPECKEY_4, SPECKEY_5},
      // FDFE
      {SPECKEY_A, SPECKEY_S, SPECKEY_D, SPECKEY_F, SPECKEY_G},
      // EFFE
      {SPECKEY_0, SPECKEY_9, SPECKEY_8, SPECKEY_7, SPECKEY_6},
      // BFFE
      {SPECKEY_ENTER, SPECKEY_L, SPECKEY_K, SPECKEY_J, SPECKEY_H},
      // 7FFE
      {SPECKEY_SPACE, SPECKEY_SYMB, SPECKEY_M, SPECKEY_N, SPECKEY_B},
      // DFFE
      {SPECKEY_P, SPECKEY_O, SPECKEY_I, SPECKEY_U, SPECKEY_Y}};

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
      {SPECKEY_SPACE, false}};

  TouchKeyboardV2(KeyEventType keyEvent) : m_keyEvent(keyEvent){};

  void start()
  {
#ifdef TOUCH_KEYBOARD_V2
    pinMode(14, OUTPUT);
    pinMode(21, OUTPUT);
    pinMode(47, OUTPUT);
    // initialise the touch pads
    setupTouchPad();
    calibrate();
    // start the task
    xTaskCreatePinnedToCore(
        keyboardTask,
        "keyboardTask",
        4096,
        this,
        1,
        NULL,
        0);
#endif
  }
};
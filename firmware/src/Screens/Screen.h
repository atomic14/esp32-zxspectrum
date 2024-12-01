#pragma once

#include <stdint.h>
#include "Emulator/keyboard_defs.h"
#include "AudioOutput/AudioOutput.h"
#include "sounds/bell.h"
#include "sounds/error.h"
#include "sounds/click.h"
#include "images/busy.h"
#include "../TFT/TFTDisplay.h"

class Display;
class AudioOutput;
class NavigationStack;

const uint8_t click[] = { 50, 50, 50, 0 };

class Screen {
  protected:
    NavigationStack *m_navigationStack = nullptr;
    Display &m_tft;
    AudioOutput *m_audioOutput;
  public:
  Screen(Display &tft, AudioOutput *audioOutput) : m_tft(tft), m_audioOutput(audioOutput) {}
  // input
  virtual void updateKey(SpecKeys key, uint8_t state) {};
  virtual void pressKey(SpecKeys key) {};
  void playKeyClick() {
    if (m_audioOutput) {
      m_audioOutput->write(click, 4);
    }
  }
  void playErrorBeep() {
    if (m_audioOutput) {
      m_audioOutput->write(fatal_error_raw, fatal_error_raw_len);
    }
  }
  void playSuccessBeep() {
    if (m_audioOutput) {
      m_audioOutput->write(bell_raw, bell_raw_len);
    }
  }
  void drawBusy() {
    m_tft.fillRect(m_tft.width() / 2 - busyImageWidth /2 - 2,
      m_tft.height() / 2 - busyImageHeight /2 - 2,
      busyImageWidth /2 + 4,
      busyImageHeight /2 + 4, TFT_BLACK);
    m_tft.setWindow(
      m_tft.width() / 2 - busyImageWidth /2,
      m_tft.height() / 2 - busyImageHeight /2,
      m_tft.width() / 2 + busyImageWidth /2 - 1,
      m_tft.height() / 2 + busyImageHeight /2 - 1);
    m_tft.pushPixels((uint16_t *) busyImageData, busyImageWidth * busyImageHeight);
  }
  // lifecycle
  virtual void didAppear() {}
  virtual void willDisappear() {}
  void setNavigationStack(NavigationStack *navigationStack) {
    m_navigationStack = navigationStack;
  }
};
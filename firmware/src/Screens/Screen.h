#pragma once

#include <stdint.h>
#include "Emulator/keyboard_defs.h"

class TFTDisplay;
class AudioOutput;
class NavigationStack;

class Screen {
  protected:
    NavigationStack *m_navigationStack = nullptr;
    TFTDisplay &m_tft;
    AudioOutput *m_audioOutput;
  public:
  Screen(TFTDisplay &tft, AudioOutput *audioOutput) : m_tft(tft), m_audioOutput(audioOutput) {}
  // input
  virtual void updatekey(SpecKeys key, uint8_t state) {};
  virtual void pressKey(SpecKeys key) {};
  // lifecycle
  virtual void didAppear() {}
  virtual void willDisappear() {}
  void setNavigationStack(NavigationStack *navigationStack) {
    m_navigationStack = navigationStack;
  }
};
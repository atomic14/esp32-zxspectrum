#pragma once

#include <stdint.h>
#include "Emulator/keyboard_defs.h"

class TFTDisplay;
class AudioOutput;

class Screen {
  protected:
    TFTDisplay &m_tft;
    AudioOutput *m_audioOutput;
  public:
  Screen(TFTDisplay &tft, AudioOutput *audioOutput) : m_tft(tft), m_audioOutput(audioOutput) {}
  // input
  virtual void updatekey(SpecKeys key, uint8_t state) = 0;
  // lifecycle
  virtual void didAppear() {}
};
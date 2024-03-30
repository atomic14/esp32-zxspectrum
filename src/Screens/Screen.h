#pragma once

#include <stdint.h>

class TFT_eSPI;
class AudioOutput;

class Screen {
  protected:
    TFT_eSPI &m_tft;
    AudioOutput *m_audioOutput;
  public:
  Screen(TFT_eSPI &tft, AudioOutput *audioOutput) : m_tft(tft), m_audioOutput(audioOutput) {}
  // input
  virtual void updatekey(uint8_t key, uint8_t state) = 0;
};
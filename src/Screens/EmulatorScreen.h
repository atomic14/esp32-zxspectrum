#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <stdint.h>
#include <string>
#include "Screen.h"

class TFT_eSPI;
class AudioOutput;
class ZXSpectrum;
class EmulatorScreen : public Screen
{
  private:
    ZXSpectrum *machine = nullptr;
    bool isRunning = false;
    xQueueHandle frameRenderTimerQueue;
    uint16_t *frameBuffer = nullptr;
  public:
    EmulatorScreen(TFT_eSPI &tft, AudioOutput *audioOutput);
    void updatekey(uint8_t key, uint8_t state);
    void run(std::string filename);
    void stop();
    friend void drawDisplay(void *pvParameters);
    friend void z80Runner(void *pvParameter);
    uint32_t cycleCount = 0;
    uint32_t frameCount = 0;
};

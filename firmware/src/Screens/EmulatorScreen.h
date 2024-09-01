#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <stdint.h>
#include <string>
#include "Screen.h"

class TFTDisplay;
class AudioOutput;
class ZXSpectrum;
class TouchKeyboard;
class EmulatorScreen : public Screen
{
  private:
    ZXSpectrum *machine = nullptr;
    bool isRunning = false;
    SemaphoreHandle_t m_displaySemaphore;
    // uint16_t *frameBuffer = nullptr;
    uint16_t *dmaBuffer1 = nullptr;
    uint16_t *dmaBuffer2 = nullptr;
    // copy of the screen so we can track changes
    uint8_t *screenBuffer = nullptr;
    bool firstDraw = false;
    FILE *audioFile = nullptr;
  public:
    EmulatorScreen(TFTDisplay &tft, AudioOutput *audioOutput);
    void updatekey(SpecKeys key, uint8_t state);
    void run(std::string filename);
    void stop();
    friend void drawDisplay(void *pvParameters);
    friend void drawScreen(EmulatorScreen *emulatorScreen);
    friend void z80Runner(void *pvParameter);
    uint32_t cycleCount = 0;
    uint32_t frameCount = 0;
};

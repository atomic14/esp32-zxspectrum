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
    uint16_t *pixelBuffer = nullptr;
    // copy of the screen so we can track changes
    uint8_t *screenBuffer = nullptr;
    uint8_t *currentScreenBuffer = nullptr;
    bool drawReady = true;
    bool firstDraw = false;
    FILE *audioFile = nullptr;
  public:
    EmulatorScreen(TFTDisplay &tft, AudioOutput *audioOutput);
    void updatekey(SpecKeys key, uint8_t state);
    void run(std::string filename);
    void run48K();
    void run128K();
    void pause();
    void resume();
    void showSaveSnapshotScreen();
    void didAppear() {
      resume();
    }
    friend void drawDisplay(void *pvParameters);
    friend void drawScreen(EmulatorScreen *emulatorScreen);
    friend void z80Runner(void *pvParameter);
    uint32_t cycleCount = 0;
    uint32_t frameCount = 0;
};

#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <stdint.h>
#include <string>
#include "Screen.h"
#include "EmulatorScreen/Renderer.h"

class TFTDisplay;
class AudioOutput;
class ZXSpectrum;
class TouchKeyboard;
class EmulatorScreen : public Screen
{
  private:
    Renderer *renderer = nullptr;
    ZXSpectrum *machine = nullptr;
    bool isRunning = false;
    int loadProgress = 0;
    FILE *audioFile = nullptr;
    void loadTape(std::string filename);
    void tapKey(SpecKeys key);
    void startLoading();
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
    friend void z80Runner(void *pvParameter);
    uint32_t cycleCount = 0;
    uint32_t frameCount = 0;
};

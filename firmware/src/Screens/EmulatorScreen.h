#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <stdint.h>
#include <string>
#include "Screen.h"
#include "EmulatorScreen/Renderer.h"
#include "EmulatorScreen/Machine.h"

class TFTDisplay;
class AudioOutput;
class ZXSpectrum;
class TouchKeyboard;
class EmulatorScreen : public Screen
{
  private:
    Renderer *renderer = nullptr;
    Machine *machine = nullptr;
    void loadTape(std::string filename);
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
};

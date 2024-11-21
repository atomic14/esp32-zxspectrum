#pragma once

#include "stdio.h"
#include <functional>
#include "../../Emulator/spectrum.h"

void runnerTask(void *pvParameter);

class Renderer;
class ZXSpectrum;
class AudioOutput;

class Machine {
  private:
    // the actual machine
    ZXSpectrum *machine = nullptr;
    // are we currently running?
    bool isRunning = false;
    // the renderer - we trigger a redraw every frame
    Renderer *renderer = nullptr;
    // where are we sending audio?
    AudioOutput *audioOutput = nullptr;
    // the task that runs the emulator and the function that it runs
    friend void runnerTask(void *pvParameter);
    void runEmulator();
    // streams audio data to a file
    FILE *audioFile = nullptr;
    // keeps track of how many tstates we've run
    uint32_t cycleCount = 0;
    // callback for when rom loading routine is hit
    std::function<void()> romLoadingRoutineHitCallback;
  public:
    Machine(Renderer *renderer, AudioOutput *audioOutput, std::function<void()> romLoadingRoutineHitCallback);
    void updatekey(SpecKeys key, uint8_t state);
    void setup(models_enum model);
    void start(FILE *audioFile);
    void pause() {
      isRunning = false;
    }
    void resume() {
      isRunning = true;
    }
    ZXSpectrum *getMachine() {
      return machine;
    }
    void tapKey(SpecKeys key);
    void startLoading();
};
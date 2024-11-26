#pragma once

#include <string>
#include "Screen.h"

class TFTDisplay;
class AudioOutput;
class ZXSpectrum;
class TouchKeyboard;
class Machine;
class GameLoader;
class Renderer;

class EmulatorScreen : public Screen
{
  private:
    Renderer *renderer = nullptr;
    Machine *machine = nullptr;
    GameLoader *gameLoader = nullptr;
    FILE *audioFile = nullptr;
    IFiles *m_files;
    void triggerLoadTape();
    bool isLoading = false;
  public:
    EmulatorScreen(TFTDisplay &tft, AudioOutput *audioOutput, IFiles *files);
    void updateKey(SpecKeys key, uint8_t state);
    void run(std::string filename, models_enum model);
    void pause();
    void resume();
    void showSaveSnapshotScreen();
    void didAppear() {
      resume();
    }
    void loadTape(std::string filename);
};

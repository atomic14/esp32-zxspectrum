#include <Arduino.h>
#include "../TFT/TFTDisplay.h"
#include "../Emulator/spectrum.h"
#include "../Emulator/snaps.h"
#include "../AudioOutput/AudioOutput.h"
#include "../Files/Files.h"
#include "EmulatorScreen.h"
#include "NavigationStack.h"
#include "SaveSnapshotScreen.h"
#include "EmulatorScreen/Renderer.h"
#include "EmulatorScreen/Machine.h"
#include "EmulatorScreen/GameLoader.h"

EmulatorScreen::EmulatorScreen(TFTDisplay &tft, AudioOutput *audioOutput) : Screen(tft, audioOutput)
{
  renderer = new Renderer(tft);
  machine = new Machine(renderer, audioOutput);
  gameLoader = new GameLoader(machine, renderer, audioOutput);
  pinMode(0, INPUT_PULLUP);
}

void EmulatorScreen::run(std::string filename, models_enum model)
{
  m_tft.fillScreen(TFT_BLACK);
  renderer->start();
  auto bl = BusyLight();
  machine->setup(model);
  if (filename.size() > 0)
  {
    // check for tap or tpz files
    std::string ext = filename.substr(filename.find_last_of(".") + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c)
                   { return std::tolower(c); });
    if (ext == "tap" || ext == "tzx")
    {
      gameLoader->loadTape(filename.c_str());
    }
    else
    {
      // generic loading of z80 and sna files
      Load(machine->getMachine(), filename.c_str());
    }
  }
  // audioFile = fopen("/fs/audio.raw", "wb");
  machine->start(audioFile);
}

void EmulatorScreen::pause()
{
  renderer->pause();
  machine->pause();
}

void EmulatorScreen::resume()
{
  renderer->resume();
  machine->resume();
}

void EmulatorScreen::updatekey(SpecKeys key, uint8_t state)
{
  // TODO audio capture
  // if (key == SPECKEY_0)
  // {
  //   if (audioFile)
  //   {
  //     machine->pause();
  //     vTaskDelay(1000 / portTICK_PERIOD_MS);
  //     fclose(audioFile);
  //     audioFile = NULL;
  //     Serial.printf("Audio file closed\n");
  //   }
  // }
  machine->updatekey(key, state);
}

void EmulatorScreen::showSaveSnapshotScreen()
{
  m_navigationStack->push(new SaveSnapshotScreen(m_tft, m_audioOutput, machine->getMachine()));
}

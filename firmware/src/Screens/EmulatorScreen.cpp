#include <Arduino.h>
#include "../TFT/TFTDisplay.h"
#include "../Emulator/spectrum.h"
#include "../Emulator/snaps.h"
#include "../AudioOutput/AudioOutput.h"
#include "../Files/Files.h"
#include "ErrorScreen.h"
#include "EmulatorScreen.h"
#include "AlphabetPicker.h"
#include "GameFilePickerScreen.h"
#include "NavigationStack.h"
#include "SaveSnapshotScreen.h"
#include "EmulatorScreen/Renderer.h"
#include "EmulatorScreen/Machine.h"
#include "EmulatorScreen/GameLoader.h"

const std::vector<std::string> tap_extensions = {".tap", ".tzx"};
const std::vector<std::string> no_sd_card_error = {"No SD Card", "Insert an SD Card", "to load games"};
const std::vector<std::string> no_files_error = {"No games found", "on the SD Card", "add Z80 or SNA files"};

void EmulatorScreen::triggerLoadTape()
{
  if (isLoading) {
    return;
  }
  machine->pause();
  renderer->pause();
  if (!m_files->isAvailable())
  {
    ErrorScreen *errorScreen = new ErrorScreen(
        no_sd_card_error,
        m_tft,
        m_hdmiDisplay,
        m_audioOutput,
        m_files);
    m_navigationStack->push(errorScreen);
    return;
  }
  drawBusy();
  FileLetterCountVector fileLetterCounts = m_files->getFileLetters("/", tap_extensions);
  if (fileLetterCounts.size() == 0)
  {
    ErrorScreen *errorScreen = new ErrorScreen(
        no_files_error,
        m_tft,
        m_hdmiDisplay,
        m_audioOutput,
        m_files);
    m_navigationStack->push(errorScreen);
    return;
  }
  AlphabetPicker<GameFilePickerScreen> *alphabetPicker = new AlphabetPicker<GameFilePickerScreen>("Select Tape File", m_files, m_tft, m_hdmiDisplay, m_audioOutput, "/", tap_extensions);
  alphabetPicker->setItems(fileLetterCounts);
  m_navigationStack->push(alphabetPicker);
}

EmulatorScreen::EmulatorScreen(Display &tft, HDMIDisplay *hdmiDisplay, AudioOutput *audioOutput, IFiles *files)
    : Screen(tft, hdmiDisplay, audioOutput, files)
{
  renderer = new Renderer(tft, audioOutput, hdmiDisplay);
  machine = new Machine(renderer, audioOutput, [&]()
                        {
    Serial.println("ROM loading routine hit");
    triggerLoadTape(); });
  gameLoader = new GameLoader(machine, renderer, audioOutput);
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
      machine->startLoading();
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

void EmulatorScreen::updateKey(SpecKeys key, uint8_t state)
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
  if (!renderer->isShowingTimeTravel)
  {
    machine->updateKey(key, state);
  }
}

void EmulatorScreen::pressKey(SpecKeys key)
{
  if (key == SPECKEY_MENU)
  {
    if (renderer->isShowingMenu) {
      renderer->isShowingMenu = false;
      renderer->forceRedraw();
      machine->resume();
    } else {
      machine->pause();
      renderer->isShowingMenu = true;
    }
  }
  if (renderer->isShowingTimeTravel)
  {
    if (key == SPECKEY_ENTER)
    {
      machine->stopTimeTravel();
      renderer->isShowingTimeTravel = false;
      renderer->forceRedraw();
      machine->resume();
    }
    else
    {
      if (key == SPECKEY_5) {
        machine->stepBack();
        renderer->forceRedraw();
      }
      if (key == SPECKEY_8) {
        machine->stepForward();
        renderer->forceRedraw();
      }
    }
  } else if (renderer->isShowingMenu) 
  {
    if (key == SPECKEY_1) {
      renderer->isShowingMenu = false;
      renderer->isShowingTimeTravel = true;
      machine->startTimeTravel();
      renderer->forceRedraw();
    }
    else if (key == SPECKEY_2) {
      renderer->isShowingMenu = false;
      // show the save snapshot UI
      m_navigationStack->push(new SaveSnapshotScreen(m_tft, m_hdmiDisplay, m_audioOutput, machine->getMachine(), m_files));
    } else if (key == SPECKEY_SPACE || key == SPECKEY_ENTER) {
      renderer->isShowingMenu = false;
      machine->resume();
      renderer->forceRedraw();
    } else if (key == SPECKEY_5) {
    m_audioOutput->volumeDown();
      renderer->forceRedraw();
    } else if (key == SPECKEY_8) {
      m_audioOutput->volumeUp();
      renderer->forceRedraw();
    }
  }
}

void EmulatorScreen::loadTape(std::string filename)
{
  isLoading = true;
  renderer->resume();
  renderer->setNeedsRedraw();
  gameLoader->loadTape(filename);
  renderer->setIsLoading(false);
  renderer->setNeedsRedraw();
  machine->resume();
  isLoading = false;
}

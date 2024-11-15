#include <Arduino.h>
#include "../TFT/TFTDisplay.h"
#include "../Emulator/spectrum.h"
#include "../Emulator/snaps.h"
#include "../AudioOutput/AudioOutput.h"
#include "../GPIOTapeLoader/GPIOTapeLoader.h"
#include "../Files/Files.h"
#include "EmulatorScreen.h"
#include "NavigationStack.h"
#include "SaveSnapshotScreen.h"
#include "../TZX/ZXSpectrumTapeListener.h"
#include "../TZX/DummyListener.h"
#include "../TZX/tzx_cas.h"
#include "utils.h"

EmulatorScreen::EmulatorScreen(TFTDisplay &tft, AudioOutput *audioOutput) : Screen(tft, audioOutput)
{
  renderer = new Renderer(tft);
  machine = new Machine(renderer, audioOutput, nullptr);
  pinMode(0, INPUT_PULLUP);
}


void EmulatorScreen::loadTape(std::string filename)
{
  ScopeGuard guard([&]()
                   {
    renderer->setIsLoading(false);
    if (m_audioOutput) m_audioOutput->resume(); });
  // stop audio playback
  if (m_audioOutput)
  {
    m_audioOutput->pause();
  }
  uint64_t startTime = get_usecs();
  renderer->setIsLoading(true);
  Serial.printf("Loading tape %s\n", filename.c_str());
  machine->startLoading();
  Serial.printf("Loading tape file\n");
  FILE *fp = fopen(filename.c_str(), "rb");
  if (fp == NULL)
  {
    Serial.println("Error: Could not open file.");
    std::cout << "Error: Could not open file." << std::endl;
    return;
  }
  fseek(fp, 0, SEEK_END);
  long file_size = ftell(fp);
  fseek(fp, 0, SEEK_SET);
  Serial.printf("File size %d\n", file_size);
  uint8_t *tzx_data = (uint8_t *)ps_malloc(file_size);
  if (!tzx_data)
  {
    Serial.println("Error: Could not allocate memory.");
    return;
  }
  fread(tzx_data, 1, file_size, fp);
  fclose(fp);
  // load the tape
  TzxCas tzxCas;
  DummyListener *dummyListener = new DummyListener();
  dummyListener->start();
  if (filename.find(".tap") != std::string::npos || filename.find(".TAP") != std::string::npos)
  {
    tzxCas.load_tap(dummyListener, tzx_data, file_size);
  }
  else
  {
    tzxCas.load_tzx(dummyListener, tzx_data, file_size);
  }
  dummyListener->finish();
  uint64_t totalTicks = dummyListener->getTotalTicks();
  Serial.printf("Total cycles: %lld\n", dummyListener->getTotalTicks());
  delete dummyListener;
  int count = 0;
  int borderPos = 0;
  uint8_t currentBorderColors[312] = {0};
  ZXSpectrumTapeListener *listener = new ZXSpectrumTapeListener(machine->getMachine(), [&](uint64_t progress)
                                                                {
        // approximate the border position - not very accutare but good enough
        // get the border color
        currentBorderColors[borderPos] = machine->getMachine()->hwopt.BorderColor & B00000111;
        borderPos++;
        count++;
        if (borderPos == 312) {
          borderPos = 0;
          renderer->triggerDraw(machine->getMachine()->mem.currentScreen, currentBorderColors);
        }
        if (count % 4000 == 0) {
          float machineTime = (float) listener->getTotalTicks() / 3500000.0f;
          float wallTime = (float) (get_usecs() - startTime) / 1000000.0f;
          Serial.printf("Total execution time: %fs\n", (float) listener->getTotalExecutionTime() / 1000000.0f);
          Serial.printf("Total machine time: %f\n", machineTime);
          Serial.printf("Wall Clock time: %fs\n", wallTime);
          Serial.printf("Speed Up: %f\n",  machineTime/wallTime);
          Serial.printf("Progress: %lld\n", progress * 100 / totalTicks);
          // draw a progreess bar
          renderer->setLoadProgress(progress * 100 / totalTicks);
          vTaskDelay(1);
        } });
  listener->start();
  if (filename.find(".tap") != std::string::npos || filename.find(".TAP") != std::string::npos)
  {
    Serial.printf("Loading tap file\n");
    tzxCas.load_tap(listener, tzx_data, file_size);
  }
  else
  {
    Serial.printf("Loading tzx file\n");
    tzxCas.load_tzx(listener, tzx_data, file_size);
  }
  Serial.printf("Tape loaded\n");
  listener->finish();
  Serial.printf("*********************");
  Serial.printf("Total execution time: %lld\n", listener->getTotalExecutionTime());
  Serial.printf("Total cycles: %lld\n", listener->getTotalTicks());
  Serial.printf("*********************");
  free(tzx_data);
  delete listener;
}

void EmulatorScreen::run(std::string filename)
{
  m_tft.fillScreen(TFT_BLACK);
  renderer->start();
  // audioFile = fopen("/fs/audio.raw", "wb");
  auto bl = BusyLight();
  // check for tap or tpz files
  std::string ext = filename.substr(filename.find_last_of(".") + 1);
  std::transform(ext.begin(), ext.end(), ext.begin(),
                 [](unsigned char c)
                 { return std::tolower(c); });
  if (ext == "tap" || ext == "tzx")
  {
    machine->setup(SPECMDL_128K);
    loadTape(filename.c_str());
    machine->start();
  }
  else
  {
    // generic loading of z80 and sna files
    machine->setup(SPECMDL_48K);
    Load(machine->getMachine(), filename.c_str());
    machine->start();
  }
}

void EmulatorScreen::run48K()
{
  m_tft.fillScreen(TFT_BLACK);
  renderer->start();
  machine->setup(SPECMDL_48K);
  machine->start();
}

void EmulatorScreen::run128K()
{
  m_tft.fillScreen(TFT_BLACK);
  renderer->start();
  machine->setup(SPECMDL_128K);
  machine->start();
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
  //     isRunning = false;
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

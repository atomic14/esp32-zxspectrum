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


void z80Runner(void *pvParameter)
{
  EmulatorScreen *emulatorScreen = (EmulatorScreen *)pvParameter;
  unsigned long lastTime = millis();
  while (1)
  {
    if (emulatorScreen->isRunning)
    {
      emulatorScreen->cycleCount += emulatorScreen->machine->runForFrame(emulatorScreen->m_audioOutput, emulatorScreen->audioFile);
      emulatorScreen->renderer->triggerDraw(emulatorScreen->machine->mem.currentScreen, emulatorScreen->machine->borderColors);
      unsigned long currentTime = millis();
      unsigned long elapsed = currentTime - lastTime;
      if (elapsed > 1000)
      {
        lastTime = currentTime;
        float cycles = emulatorScreen->cycleCount / (elapsed * 1000.0);
        float fps = emulatorScreen->frameCount / (elapsed / 1000.0);
        Serial.printf("Executed at %.3FMHz cycles, frame rate=%.2f\n", cycles, fps);
        emulatorScreen->frameCount = 0;
        emulatorScreen->cycleCount = 0;
      }
    }
    else
    {
      vTaskDelay(100 / portTICK_PERIOD_MS);
    }
  }
}

EmulatorScreen::EmulatorScreen(TFTDisplay &tft, AudioOutput *audioOutput) : Screen(tft, audioOutput)
{
  renderer = new Renderer(tft);
  pinMode(0, INPUT_PULLUP);
}

void EmulatorScreen::startLoading()
{
  for (int i = 0; i < 200; i++)
  {
    machine->runForFrame(nullptr, nullptr);
  }
  renderer->triggerDraw(machine->mem.currentScreen, machine->borderColors);
  if (machine->hwopt.hw_model == SPECMDL_48K)
  {
    tapKey(SPECKEY_J);
    machine->updatekey(SPECKEY_SYMB, 1);
    tapKey(SPECKEY_P);
    tapKey(SPECKEY_P);
    machine->updatekey(SPECKEY_SYMB, 0);
    tapKey(SPECKEY_ENTER);
  }
  else
  {
    // 128K the tape loader is first in the menu
    tapKey(SPECKEY_ENTER);
  }
  renderer->triggerDraw(machine->mem.currentScreen, machine->borderColors);
}

void EmulatorScreen::tapKey(SpecKeys key)
{
  machine->updatekey(key, 1);
  for (int i = 0; i < 10; i++)
  {
    machine->runForFrame(nullptr, nullptr);
  }
  machine->updatekey(key, 0);
  for (int i = 0; i < 10; i++)
  {
    machine->runForFrame(nullptr, nullptr);
  }
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
  startLoading();
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
  ZXSpectrumTapeListener *listener = new ZXSpectrumTapeListener(machine, [&](uint64_t progress)
                                                                {
        // approximate the border position - not very accutare but good enough
        // get the border color
        currentBorderColors[borderPos] = machine->hwopt.BorderColor & B00000111;
        borderPos++;
        count++;
        if (borderPos == 312) {
          borderPos = 0;
          renderer->triggerDraw(machine->mem.currentScreen, currentBorderColors);
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
          loadProgress = progress * 100 / totalTicks;
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
  machine = new ZXSpectrum();
  machine->reset();
  // check for tap or tpz files
  std::string ext = filename.substr(filename.find_last_of(".") + 1);
  std::transform(ext.begin(), ext.end(), ext.begin(),
                 [](unsigned char c)
                 { return std::tolower(c); });
  if (ext == "tap" || ext == "tzx")
  {
    machine->init_spectrum(SPECMDL_48K);
    machine->reset_spectrum(machine->z80Regs);
    loadTape(filename.c_str());
  }
  else
  {
    // generic loading of z80 and sna files
    machine->init_spectrum(SPECMDL_48K);
    machine->reset_spectrum(machine->z80Regs);
    Load(machine, filename.c_str());
  }
  // tasks to do the work
  Serial.println("Starting tasks\n");
  isRunning = true;
  xTaskCreatePinnedToCore(z80Runner, "z80Runner", 8192, this, 5, NULL, 0);
}

void EmulatorScreen::run48K()
{
  m_tft.fillScreen(TFT_BLACK);
  renderer->start();
  machine = new ZXSpectrum();
  machine->reset();
  machine->init_spectrum(SPECMDL_48K);
  machine->reset_spectrum(machine->z80Regs);
  m_tft.fillScreen(TFT_WHITE);
  isRunning = true;
  // tasks to do the work
  xTaskCreatePinnedToCore(z80Runner, "z80Runner", 8192, this, 5, NULL, 0);
}

void EmulatorScreen::run128K()
{
  m_tft.fillScreen(TFT_BLACK);
  renderer->start();
  machine = new ZXSpectrum();
  machine->reset();
  machine->init_spectrum(SPECMDL_128K);
  machine->reset_spectrum(machine->z80Regs);
  m_tft.fillScreen(TFT_WHITE);
  // tasks to do the work
  xTaskCreatePinnedToCore(z80Runner, "z80Runner", 8192, this, 5, NULL, 0);
}

void EmulatorScreen::pause()
{
  isRunning = false;
  renderer->pause();
}

void EmulatorScreen::resume()
{
  isRunning = true;
  renderer->resume();
}

void EmulatorScreen::updatekey(SpecKeys key, uint8_t state)
{
  if (key == SPECKEY_0)
  {
    if (audioFile)
    {
      isRunning = false;
      vTaskDelay(1000 / portTICK_PERIOD_MS);
      fclose(audioFile);
      audioFile = NULL;
      Serial.printf("Audio file closed\n");
    }
  }
  if (isRunning)
  {
    machine->updatekey(key, state);
  }
}

void EmulatorScreen::showSaveSnapshotScreen()
{
  m_navigationStack->push(new SaveSnapshotScreen(m_tft, m_audioOutput, machine));
}

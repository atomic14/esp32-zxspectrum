#include <iostream>
#include "../../TZX/ZXSpectrumTapeListener.h"
#include "../../TZX/DummyListener.h"
#include "../../TZX/tzx_cas.h"
#include "./Machine.h"
#include "./GameLoader.h"
#include "./Renderer.h"
#include "../../AudioOutput/AudioOutput.h"
#include "../../utils.h"

GameLoader::GameLoader(Machine *machine, Renderer *renderer, AudioOutput *audioOutput) : machine(machine), renderer(renderer), audioOutput(audioOutput) {}

void GameLoader::loadTape(std::string filename)
{
  ScopeGuard guard([&]()
                   {
    renderer->setIsLoading(false);
    if (audioOutput) audioOutput->resume(); });
  // stop audio playback
  if (audioOutput)
  {
    audioOutput->pause();
  }
  uint64_t startTime = get_usecs();
  renderer->setIsLoading(true);
  Serial.printf("Loading tape %s\n", filename.c_str());
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
          renderer->triggerDraw(machine->getMachine()->mem.currentScreen->data, currentBorderColors);
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
#include "./Machine.h"
#include "./Renderer.h"
#include "../../AudioOutput/AudioOutput.h"

void runnerTask(void *pvParameter)
{
  Machine *machine = (Machine *)pvParameter;
  machine->runEmulator();
}

void Machine::runEmulator() {
  unsigned long lastTime = millis();
  while (1)
  {
    if (isRunning)
    {
      cycleCount += machine->runForFrame(audioOutput, audioFile);
      renderer->triggerDraw(machine->mem.currentScreen, machine->borderColors);
      unsigned long currentTime = millis();
      unsigned long elapsed = currentTime - lastTime;
      if (elapsed > 1000)
      {
        lastTime = currentTime;
        float cycles = cycleCount / (elapsed * 1000.0);
        float fps = renderer->getFrameCount() / (elapsed / 1000.0);
        Serial.printf("Executed at %.3FMHz cycles, frame rate=%.2f\n", cycles, fps);
        renderer->resetFrameCount();
        cycleCount = 0;
      }
      if (machine->romLoadingRoutineHit)
      {
        romLoadingRoutineHitCallback();
      }
    }
    else
    {
      vTaskDelay(100 / portTICK_PERIOD_MS);
    }
  }
}


Machine::Machine(Renderer *renderer, AudioOutput *audioOutput, std::function<void()> romLoadingRoutineHitCallback)
: renderer(renderer), audioOutput(audioOutput), romLoadingRoutineHitCallback(romLoadingRoutineHitCallback) {
  Serial.println("Creating machine");
  machine = new ZXSpectrum();
}

void Machine::updatekey(SpecKeys key, uint8_t state) {
  if (isRunning) {
    machine->updatekey(key, state);
  }
}

void Machine::setup(models_enum model) {
  Serial.println("Setting up machine");
  machine->reset();
  machine->init_spectrum(model);
  machine->reset_spectrum(machine->z80Regs);
}

void Machine::start(FILE *audioFile) {
  Serial.println("Starting machine");
  this->audioFile = audioFile;
  isRunning = true;
  xTaskCreatePinnedToCore(runnerTask, "z80Runner", 8192, this, 5, NULL, 0);
}

void Machine::tapKey(SpecKeys key)
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


void Machine::startLoading()
{
  for (int i = 0; i < 200; i++)
  {
    machine->runForFrame(nullptr, nullptr);
  }
  renderer->triggerDraw(machine->mem.currentScreen, machine->borderColors);
  // TODO load screenshot...
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
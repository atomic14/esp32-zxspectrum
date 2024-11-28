#pragma once

#include "stdio.h"
#include <functional>
#include <list>
#include <vector>
#include <deque>
#include "Renderer.h"
#include "../../Emulator/spectrum.h"
#include "../../Serial.h"

void runnerTask(void *pvParameter);

class Renderer;
class ZXSpectrum;
class AudioOutput;

// handles the time travel functionality - this captures the state of the machine at a point in time
struct MemoryBank {
  // the index
  uint8_t index = 0;
  // the memory bank
  uint8_t *data = nullptr;
};

struct TimeTravelInstant {
  // the current memory pages
  uint8_t hwBank = 0;
  // the memory banks
  std::vector<MemoryBank *> memoryBanks;
  // the z80 registers
  Z80Regs z80Regs;
  // the border colors
  uint8_t borderColors[312] = {0};
};

class TimeTravel {
private:
  // list of time travel instants that have been recorded
  std::deque<TimeTravelInstant *> timeTravelInstants;
  // pool of memory banks that we can reuse
  std::list<MemoryBank *> memoryBanks;
  bool ensureMemoryBanks(size_t required) {
    while(memoryBanks.size() < required && timeTravelInstants.size() > 0) {
      TimeTravelInstant *oldestInstant = timeTravelInstants.front();
      timeTravelInstants.pop_front();
      for (MemoryBank *memoryBank : oldestInstant->memoryBanks) {
        memoryBanks.push_back(memoryBank);
      }
      delete oldestInstant;
    }
    return memoryBanks.size() >= required;
  }
public:
  TimeTravel() {
    // allocate some memory banks
    for (int i = 0; i < 240; i++) {
      MemoryBank *memoryBank = new MemoryBank();
      if (!memoryBank) {
        Serial.println("Could not allocate memory bank");
        return;
      }
      memoryBank->data = (uint8_t *) ps_malloc(0x4000);
      if (!memoryBank->data) {
        Serial.println("Could not allocate memory bank data");
        return;
      }
      memoryBanks.push_back(memoryBank);
    }
  }
  size_t size() {
    return timeTravelInstants.size();
  }
  // record the current state of the machine
  bool record(ZXSpectrum *machine) {
    // how many memory banks do we need?
    int memoryBankCount = 0;
    for(int i = 0; i<8; i++) {
      if (machine->mem.banks[i]->isDirty) {
        memoryBankCount++;
      }
    }
    ensureMemoryBanks(memoryBankCount);
    Serial.printf("Saving %d memory banks\n", memoryBankCount);
    // create a new time travel instant
    TimeTravelInstant *instant = new TimeTravelInstant();
    // copy the memory banks
    for(int i = 0; i<8; i++) {
      if (machine->mem.banks[i]->isDirty) {
        MemoryBank *memoryBank = memoryBanks.front();
        memoryBanks.pop_front();
        memoryBank->index = i;
        memcpy(memoryBank->data, machine->mem.banks[i]->data, 0x4000);
        machine->mem.banks[i]->isDirty = false;
        instant->memoryBanks.push_back(memoryBank);
      }
    }
    // copy the z80 registers
    memcpy(&instant->z80Regs, machine->z80Regs, sizeof(Z80Regs));
    // copy the border colors
    memcpy(instant->borderColors, machine->borderColors, 312);
    // make sure the correct paging is set
    instant->hwBank = machine->mem.hwBank;
    // add the instant to the list
    timeTravelInstants.push_back(instant);
    // if we have more than 30 seconds of time travel, remove the oldest instant
    if (timeTravelInstants.size() > 30) {
      TimeTravelInstant *oldestInstant = timeTravelInstants.front();
      timeTravelInstants.pop_front();
      for (MemoryBank *memoryBank : oldestInstant->memoryBanks) {
        memoryBanks.push_back(memoryBank);
      }
      delete oldestInstant;
    }
    Serial.printf("Recorded time travel instant %d\n", timeTravelInstants.size());
    return true;
  }
  // rewind the machine to a previous state
  void rewind(ZXSpectrum *machine, int index) {
    TimeTravelInstant *instant = timeTravelInstants[index];
    // copy the memory banks
    for (MemoryBank *memoryBank : instant->memoryBanks) {
      memcpy(machine->mem.banks[memoryBank->index]->data, memoryBank->data, 0x4000);
    }
    // copy the z80 registers
    memcpy(machine->z80Regs, &instant->z80Regs, sizeof(Z80Regs));
    // copy the border colors
    memcpy(machine->borderColors, instant->borderColors, 312);
    // make sure the correct paging is set
    machine->mem.page(instant->hwBank, true);
  }
  // remove everything from this point in time
  void reset(int index) {
    while(timeTravelInstants.size() > index) {
      TimeTravelInstant *instant = timeTravelInstants.back();
      timeTravelInstants.pop_back();
      for (MemoryBank *memoryBank : instant->memoryBanks) {
        memoryBanks.push_back(memoryBank);
      }
      delete instant;
    }
  }
};

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
    // time travel
    TimeTravel *timeTravel;
    // current time travel position
    int timeTravelPosition = 0;
    // callback for when rom loading routine is hit
    std::function<void()> romLoadingRoutineHitCallback;
  public:
    Machine(Renderer *renderer, AudioOutput *audioOutput, std::function<void()> romLoadingRoutineHitCallback);
    void updateKey(SpecKeys key, uint8_t state);
    void setup(models_enum model);
    void start(FILE *audioFile);
    void pause() {
      isRunning = false;
    }
    void resume() {
      isRunning = true;
    }
    void startTimeTravel() {
      // record the current state
      timeTravel->record(machine);
      timeTravelPosition = timeTravel->size() - 1;
      Serial.printf("Starting time travel %d\n", timeTravelPosition);
    }
    void stepBack() {
      if (timeTravelPosition > 0) {
        timeTravelPosition--;
        timeTravel->rewind(machine, timeTravelPosition);
        renderer->forceRedraw(machine->mem.currentScreen->data, machine->borderColors);
        Serial.printf("Time travel %d\n", timeTravelPosition);
      }
    }
    void stepForward() {
      if (timeTravelPosition < timeTravel->size() - 1) {
        timeTravelPosition++;
        timeTravel->rewind(machine, timeTravelPosition);
        renderer->forceRedraw(machine->mem.currentScreen->data, machine->borderColors);
        Serial.printf("Time travel %d\n", timeTravelPosition);
      }
    }
    void stopTimeTravel() {
      timeTravel->reset(timeTravelPosition);
    }
    ZXSpectrum *getMachine() {
      return machine;
    }
    void tapKey(SpecKeys key);
    void startLoading();
};
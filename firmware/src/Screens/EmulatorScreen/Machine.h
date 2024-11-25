#pragma once

#include "stdio.h"
#include <functional>
#include <list>
#include <vector>
#include <deque>
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
  std::vector<MemoryBank *> memoryBanks;
  Z80Regs z80Regs;
  uint8_t borderColors[32] = {0};
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
    memcpy(instant->borderColors, machine->borderColors, 32);
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
      memcpy(machine->mem.mappedMemory[memoryBank->index], memoryBank->data, 0x4000);
    }
    // copy the z80 registers
    memcpy(machine->z80Regs, &instant->z80Regs, sizeof(Z80Regs));
    // copy the border colors
    memcpy(machine->borderColors, instant->borderColors, 32);
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
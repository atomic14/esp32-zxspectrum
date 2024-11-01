#pragma once

#include <stdint.h>

#define CPU_FREQ 3500000
#define MILLI_SECOND (CPU_FREQ / 1000)

class TapeListener {
protected:
  uint64_t totalTicks = 0;
public:
  virtual void start() = 0;
  virtual void toggleMicLevel() = 0;
  virtual void setMicHigh() = 0;
  virtual void setMicLow() = 0;
  virtual void runForTicks(uint32_t ticks) = 0;
  virtual void pause1Millis() = 0;
  virtual void finish() = 0;
  uint64_t getTotalTicks() {
    return totalTicks;
  }
};
#pragma once

#include <stdint.h>
#include <functional>

#define CPU_FREQ 3500000
#define MILLI_SECOND (CPU_FREQ / 1000)

class TapeListener {
protected:
  uint64_t totalTicks = 0;
  uint64_t elapsedTicks = 0;
  using ProgressEvent = std::function<void(uint64_t ticks)>;
  ProgressEvent progressEvent;
public:
  TapeListener(ProgressEvent progressEvent) : progressEvent(progressEvent) {}
  virtual void start() = 0;
  virtual void toggleMicLevel() = 0;
  virtual void setMicHigh() = 0;
  virtual void setMicLow() = 0;
  virtual void runForTicks(uint64_t ticks) = 0;
  virtual void pause1Millis() = 0;
  virtual void finish() = 0;
  uint64_t getTotalTicks() {
    return totalTicks;
  }
  inline virtual void addTicks(uint64_t ticks) {
    totalTicks += ticks;
    elapsedTicks += ticks;
    if (elapsedTicks >= 10*224) {
      elapsedTicks = 0;
      if (progressEvent) {
        progressEvent(totalTicks);
      }
    }
  }
};
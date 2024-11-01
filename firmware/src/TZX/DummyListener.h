#pragma once

#include "TapeListener.h"

class DummyListener: public TapeListener {
  public:
    void start() {
      totalTicks = 0;
    }
    void toggleMicLevel() {
    }
    void setMicHigh() {
    }
    void setMicLow() {
    }
    void runForTicks(uint32_t ticks) {
      totalTicks += ticks;
    }
    void pause1Millis() {
      totalTicks += MILLI_SECOND;
    }
  void finish() {
  }
};
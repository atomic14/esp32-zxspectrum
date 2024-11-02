#pragma once

#include "TapeListener.h"

class DummyListener: public TapeListener {
  public:
    DummyListener() : TapeListener(nullptr) {}
    void start() {
      totalTicks = 0;
    }
    void toggleMicLevel() {
    }
    void setMicHigh() {
    }
    void setMicLow() {
    }
    void runForTicks(uint64_t ticks) {
      addTicks(ticks);
    }
    void pause1Millis() {
      addTicks(MILLI_SECOND);
    }
  void finish() {
  }
};
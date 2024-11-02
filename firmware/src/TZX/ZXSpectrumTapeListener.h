#include "Serial.h"
#include "../Emulator/spectrum.h"
#include "TapeListener.h"

class ZXSpectrumTapeListener:public TapeListener {
  private:
    ZXSpectrum *spectrum;
    uint64_t totalExecutionTime = 0;
  public:
  ZXSpectrumTapeListener(ZXSpectrum *spectrum, ProgressEvent progressEvent) : TapeListener(progressEvent) {
    this->spectrum = spectrum;
  }
  virtual void start() {
    // nothing to do - maybe we could start the spectrum tape loader?
  }
  inline virtual void toggleMicLevel() {
    this->spectrum->toggleMicLevel();
  }
  inline virtual void setMicHigh() {
    this->spectrum->setMicHigh();
  }
  inline virtual void setMicLow() {
    this->spectrum->setMicLow();
  }
  inline virtual void runForTicks(uint64_t ticks) {
    addTicks(ticks);
    uint64_t startUs = get_usecs();
    this->spectrum->runForCycles(ticks);
    uint64_t endUs = get_usecs();
    totalExecutionTime += endUs - startUs;
  }
  inline virtual void pause1Millis() {
    addTicks(MILLI_SECOND);
    uint64_t startUs = get_usecs();
    this->spectrum->runForCycles(MILLI_SECOND);
    uint64_t endUs = get_usecs();
    totalExecutionTime += endUs - startUs;
  }
  virtual void finish() {
    // what should we do here?
  }
  uint64_t getTotalExecutionTime() {
    return totalExecutionTime;
  }
};

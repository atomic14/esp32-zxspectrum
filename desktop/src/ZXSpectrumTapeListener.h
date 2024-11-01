#include "spectrum.h"
#include "TapeListener.h"

class ZXSpectrumTapeListener:public TapeListener {
  private:
    ZXSpectrum *spectrum;
  public:
  ZXSpectrumTapeListener(ZXSpectrum *spectrum) {
    this->spectrum = spectrum;
  }
  virtual void start() {
    // nothing to do - maybe we could start the spectrum tape loader?
  }
  virtual void toggleMicLevel() {
    this->spectrum->toggleMicLevel();
  }
  virtual void setMicHigh() {
    this->spectrum->setMicHigh();
  }
  virtual void setMicLow() {
    this->spectrum->setMicLow();
  }
  virtual void runForTicks(uint32_t ticks) {
    totalTicks += ticks;
    this->spectrum->runForCycles(ticks);
  }
  virtual void pause1Millis() {
    totalTicks += MILLI_SECOND;
    this->spectrum->runForCycles(MILLI_SECOND);
  }
  virtual void finish() {
    // what should we do here?
  }
  uint64_t getTotalTicks() {
    return totalTicks;
  }
};

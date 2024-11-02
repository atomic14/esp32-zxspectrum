#pragma once

#include "TapeListener.h"
#include "stdio.h"
#include <string>

#define TZX_WAV_FREQUENCY 44100
#define TICKS_TO_SAMPLES(ticks) (ticks * TZX_WAV_FREQUENCY / CPU_FREQ)
#define WAVE_LOW -0x5a9e
#define WAVE_HIGH 0x5a9e
#define WAVE_NULL 0

class RawAudioListener: public TapeListener {
  private:
    int micLevel = WAVE_NULL;
    FILE *fp = NULL;
    std::string fname;
  public:
    RawAudioListener(const char *fname): TapeListener(NULL) {
      this->fname = fname;
    }
    void start() {
      fp = fopen(fname.c_str(), "wb");
      totalTicks = 0;
    }
    void toggleMicLevel() {
      if (micLevel == WAVE_LOW) {
        micLevel = WAVE_HIGH;
      } else {
        micLevel = WAVE_LOW;
      }
    }
    void setMicHigh() {
      micLevel = WAVE_HIGH;
    }
    void setMicLow() {
      micLevel = WAVE_LOW;
    }
    void runForTicks(uint64_t ticks) {
      totalTicks += ticks;
      // need to write out the number of samples that correspond to these CPU ticks
      // the cpu runs at 3.5MHz
      uint32_t samples = TICKS_TO_SAMPLES(ticks);
      for(uint32_t i = 0; i < samples; i++) {
        fwrite(&micLevel, 1, 2, fp);
      }
    }
    void pause1Millis() {
      // no need to add anything to totalTicks - this is donein runForTicks
      runForTicks(MILLI_SECOND);
    }
  void finish() {
    fclose(fp);
  }
};
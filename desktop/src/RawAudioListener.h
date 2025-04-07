#pragma once

#include "TapeListener.h"
#include "stdio.h"
#include <string>
#include <vector>

#define TZX_WAV_FREQUENCY 44100
#define TICKS_TO_SAMPLES(ticks) (ticks * TZX_WAV_FREQUENCY / CPU_FREQ)
#define WAVE_LOW -0x5a9e
#define WAVE_HIGH 0x5a9e
#define WAVE_NULL 0

class RawAudioListener: public TapeListener {
  private:
    int micLevel = WAVE_NULL;
    std::vector<uint8_t>& wavFile;
    size_t dataOffset;  // Offset where audio data starts
    
  public:
    RawAudioListener(std::vector<uint8_t>& buffer): TapeListener(NULL), wavFile(buffer) {
      // Initialize WAV header
      const uint16_t numChannels = 1;
      const uint32_t sampleRate = TZX_WAV_FREQUENCY;
      const uint16_t bitsPerSample = 16;
      const uint16_t blockAlign = numChannels * (bitsPerSample / 8);
      const uint32_t byteRate = sampleRate * blockAlign;
      uint32_t tempDataSize = 0;  // Will be updated later
      
      wavFile.clear();
      wavFile.reserve(1024 * 1024);  // Reserve 1MB to start (will grow if needed)
      
      // RIFF header
      appendBytes("RIFF", 4);
      appendBytes(&tempDataSize, 4);  // File size (updated later)
      appendBytes("WAVE", 4);
      
      // fmt chunk
      appendBytes("fmt ", 4);
      uint32_t fmtSize = 16;
      appendBytes(&fmtSize, 4);
      uint16_t audioFormat = 1;  // PCM
      appendBytes(&audioFormat, 2);
      appendBytes(&numChannels, 2);
      appendBytes(&sampleRate, 4);
      appendBytes(&byteRate, 4);
      appendBytes(&blockAlign, 2);
      appendBytes(&bitsPerSample, 2);
      
      // data chunk header
      appendBytes("data", 4);
      appendBytes(&tempDataSize, 4);  // Data size (updated later)
      
      dataOffset = wavFile.size();  // Remember where audio data starts
    }
    
    void start() {
      totalTicks = 0;
    }

    void toggleMicLevel() {
      micLevel = (micLevel == WAVE_LOW) ? WAVE_HIGH : WAVE_LOW;
    }

    void setMicHigh() { micLevel = WAVE_HIGH; }
    void setMicLow() { micLevel = WAVE_LOW; }

    void runForTicks(uint64_t ticks) {
      totalTicks += ticks;
      uint32_t numSamples = TICKS_TO_SAMPLES(ticks);
      for(uint32_t i = 0; i < numSamples; i++) {
        appendBytes(&micLevel, sizeof(int16_t));
      }
    }

    void pause1Millis() {
      runForTicks(MILLI_SECOND);
    }

    void finish() {
      // Update file size and data size in header
      uint32_t dataSize = wavFile.size() - dataOffset;
      uint32_t fileSize = wavFile.size() - 8;  // Total size - 8 bytes for RIFF header
      
      // Update file size (offset 4)
      memcpy(wavFile.data() + 4, &fileSize, 4);
      
      // Update data size (offset 40)
      memcpy(wavFile.data() + 40, &dataSize, 4);
    }

  private:
    void appendBytes(const void* data, size_t size) {
      const uint8_t* bytes = static_cast<const uint8_t*>(data);
      wavFile.insert(wavFile.end(), bytes, bytes + size);
    }
};
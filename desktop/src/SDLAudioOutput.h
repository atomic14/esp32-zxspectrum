#pragma once
#include "AudioOutput.h"
#include "spectrum.h"
#include <thread>
#include <chrono>

// Define constants
constexpr size_t INPUT_SIZE = 312;         // Fixed input size
constexpr size_t OUTPUT_SIZE = 882;       // Fixed output size (calculated based on resampling ratio)
constexpr float INPUT_RATE = 15600.0f;    // Input sample rate
constexpr float OUTPUT_RATE = 44100.0f;   // Output sample rate

class SDLAudioOutput : public AudioOutput
{
protected:
  ZXSpectrum *mMachine;
  SDL_AudioDeviceID audioDevice;
  uint32_t buffer_size;
  // Resample uint8_t audio data with fixed input and output buffer sizes
  void resample(const uint8_t* input, uint8_t* output) {
      // Calculate the resampling ratio
      const float resampleRatio = OUTPUT_RATE / INPUT_RATE;

      // Perform linear interpolation
      for (size_t i = 0; i < OUTPUT_SIZE; ++i) {
          // Map the output index to the input index
          float inputIdx = i / resampleRatio;
          size_t idx = static_cast<size_t>(inputIdx);
          float frac = inputIdx - idx;

          // Interpolate between the two nearest samples
          if (idx < INPUT_SIZE - 1) {
              output[i] = static_cast<uint8_t>(
                  (1.0f - frac) * input[idx] + frac * input[idx + 1]
              );
          } else {
              // Handle the last sample
              output[i] = input[idx];
          }
      }
  }
  uint8_t audioBuffer[INPUT_SIZE] = {0};
public:
  SDLAudioOutput(ZXSpectrum *machine) : mMachine(machine)
  {
  };
  virtual ~SDLAudioOutput() {
    SDL_CloseAudioDevice(audioDevice);
  }
  static void fillAudioBuffer(void *userdata, uint8_t *stream, int len) {
    SDLAudioOutput *audioOutput = static_cast<SDLAudioOutput *>(userdata);
    // run the machine for a frame - this will populate the audio buffer
    audioOutput->mMachine->runForFrame(audioOutput, nullptr);
    // resample the audio to 44100
    uint8_t resampledBuffer[OUTPUT_SIZE] = {0};
    audioOutput->resample(audioOutput->audioBuffer, resampledBuffer);
    // and copy it to the SDL audio buffer
    memcpy(stream, resampledBuffer, len);
  }
  virtual void start(uint32_t dummy) {
    SDL_AudioSpec desiredSpec;
    SDL_zero(desiredSpec);
    desiredSpec.freq = OUTPUT_RATE;
    desiredSpec.format = AUDIO_U8;
    desiredSpec.channels = 1;
    desiredSpec.samples = OUTPUT_SIZE;
    desiredSpec.callback = fillAudioBuffer;
    desiredSpec.userdata = this;

    SDL_AudioSpec obtainedSpec;

    audioDevice = SDL_OpenAudioDevice(NULL, 0, &desiredSpec, &obtainedSpec, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE);
    if (audioDevice == 0) {
        std::cerr << "SDL_OpenAudioDevice failed: " << SDL_GetError() << std::endl;
        SDL_Quit();
    }
    printf("Opened audio device\n");
    printf("Desired Audio Sample Rate is: %d\n", desiredSpec.freq);
    printf("Desired Audio Format is: %d\n", desiredSpec.format);

    printf("Audio Sample Rate is: %d\n", obtainedSpec.freq);
    printf("Audio Format is: %d\n", obtainedSpec.format);
    printf("Audio Channels is: %d\n", obtainedSpec.channels);
    printf("Audio size is: %d\n", obtainedSpec.samples);

    // Start audio playback
    SDL_PauseAudioDevice(audioDevice, 0);
  }
  virtual void stop() {
    SDL_CloseAudioDevice(audioDevice);
  }
  virtual void pause() {
    SDL_PauseAudioDevice(audioDevice, 1);
  }
  virtual void resume() {
    SDL_PauseAudioDevice(audioDevice, 0);
  }
  // override this in derived classes to turn the sample into
  // something the output device expects - for the default case
  // this is simply a pass through
  virtual int16_t process_sample(int16_t sample) { return sample; }
  virtual void write(const uint8_t *samples, int count) {
    memcpy(audioBuffer, samples, count);
  }

  void setVolume(int volume){
    if (volume > 10 || volume < 0) mVolume = 10;
    else mVolume = volume;
  }

  void volumeUp() {
    if (mVolume == 10) {
      return;
    }
    mVolume++;
  }
  void volumeDown() {
    if (mVolume == 0) {
      return;
    }
    mVolume--;
  }
  int getVolume() {
    return mVolume;
  }
};

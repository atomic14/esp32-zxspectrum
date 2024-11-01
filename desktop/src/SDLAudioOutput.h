#pragma once
#include "AudioOutput.h"
#include <thread>
#include <chrono>

class SDLAudioOutput : public AudioOutput
{
protected:
  SDL_AudioDeviceID audioDevice;
  uint32_t buffer_size;
public:
  SDLAudioOutput(uint32_t buffer_size): buffer_size(buffer_size) {

  };
  virtual void start(uint32_t sample_rate) {
        SDL_AudioSpec desiredSpec;
    SDL_zero(desiredSpec);
    desiredSpec.freq = sample_rate;
    desiredSpec.format = AUDIO_U8;
    desiredSpec.channels = 1;
    desiredSpec.samples = buffer_size;

    audioDevice = SDL_OpenAudioDevice(NULL, 0, &desiredSpec, NULL, 0);
    if (audioDevice == 0) {
        std::cerr << "SDL_OpenAudioDevice failed: " << SDL_GetError() << std::endl;
        SDL_Quit();
    }
    // Start audio playback
    SDL_PauseAudioDevice(audioDevice, 0);
  }
  virtual void stop() {}
  // override this in derived classes to turn the sample into
  // something the output device expects - for the default case
  // this is simply a pass through
  virtual int16_t process_sample(int16_t sample) { return sample; }
  virtual void write(const uint8_t *samples, int count) {
    // Block until the queue has space for more samples
    while (SDL_GetQueuedAudioSize(audioDevice) > Uint32(count)) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1)); // Small delay to avoid busy-waiting
    }

    // Queue the buffer for playback
    if (SDL_QueueAudio(audioDevice, samples, count) < 0) {
      std::cerr << "SDL_QueueAudio failed: " << SDL_GetError() << std::endl;
    }
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

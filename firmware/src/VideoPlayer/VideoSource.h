
#pragma once

#include <Arduino.h>
#include "VideoPlayerState.h"

class AVIParser;

class VideoSource
{
private:
  VideoPlayerState mState = VideoPlayerState::STOPPED;
  int mAudioTimeMs = 0;
  int mLastAudioTimeUpdateMs = 0;
  int mFrameCount = 0;
  AVIParser *audioParser = nullptr;
  AVIParser *videoParser = nullptr;

public:
  VideoSource(const char *aviFilename);
  ~VideoSource();
  bool getVideoFrame(uint8_t **buffer, size_t &bufferLength, size_t &frameLength);
  int getAudioSamples(uint8_t **buffer, size_t &bufferSize, int currentAudioSample);
  // update the audio time
  void updateAudioTime(int audioTimeMs)
  {
    mAudioTimeMs = audioTimeMs;
    mLastAudioTimeUpdateMs = millis();
  }
  void setState(VideoPlayerState state)
  {
    mState = state;
    switch (state)
    {
    case VideoPlayerState::PLAYING:
      mAudioTimeMs = 0;
      mLastAudioTimeUpdateMs = millis();
      break;
    case VideoPlayerState::PAUSED:
      break;
    case VideoPlayerState::STOPPED:
      mLastAudioTimeUpdateMs = 0;
      break;
    }
  }
};

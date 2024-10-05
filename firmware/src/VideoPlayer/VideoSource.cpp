
#include <Arduino.h>
#include "VideoSource.h"
#include "AVIParser.h"

#define DEFAULT_FPS 15

VideoSource::VideoSource(const char *aviFilename)
{
    // open the AVI file
  Serial.printf("Opening AVI file %s\n", aviFilename);
  audioParser = new AVIParser(aviFilename, AVIChunkType::AUDIO);
  if (!audioParser->open()) {
    Serial.printf("Failed to open AVI file %s\n", aviFilename);
    delete audioParser;
    audioParser = NULL;
  }
  videoParser = new AVIParser(aviFilename, AVIChunkType::VIDEO);
  if (!videoParser->open()) {
    Serial.printf("Failed to open AVI file %s\n", aviFilename);
    delete videoParser;
    videoParser = NULL;
    delete audioParser;
    audioParser = NULL;
  }
}

VideoSource::~VideoSource()
{
  if (audioParser) {
    delete audioParser;
  }
  if (videoParser) {
    delete videoParser;
  }
}

bool VideoSource::getVideoFrame(uint8_t **buffer, size_t &bufferLength, size_t &frameLength)
{
  if (!videoParser) {
    return false;
  }
  if (mState == VideoPlayerState::STOPPED || mState == VideoPlayerState::STATIC)
  {
    vTaskDelay(100 / portTICK_PERIOD_MS);
    Serial.println("SDCardVideoSource::getVideoFrame: video is stopped or static");
    return false;
  }
  if (mState == VideoPlayerState::PAUSED)
  {
    // video time is not passing, so keep moving the start time forward so it is now
    mLastAudioTimeUpdateMs = millis();
    vTaskDelay(100 / portTICK_PERIOD_MS);
    Serial.println("SDCardVideoSource::getVideoFrame: video is paused");
    return false;
  }
  // work out the video time from a combination of the currentAudioSample and the elapsed time
  int elapsedTime = millis() - mLastAudioTimeUpdateMs;
  int videoTime = mAudioTimeMs + elapsedTime;
  int frameTime = 1000 * mFrameCount / DEFAULT_FPS;
  if (videoTime <= frameTime)
  {
    return false;
  }
  while (videoTime > 1000 * mFrameCount / DEFAULT_FPS)
  {
    mFrameCount++;
    frameLength = videoParser->getNextChunk((uint8_t **)buffer, bufferLength);
  }
  return true;
}

int VideoSource::getAudioSamples(uint8_t **buffer, size_t &bufferSize, int currentAudioSample)
{
  // read the audio data into the buffer
  if (audioParser) {
    int audioLength = audioParser->getNextChunk((uint8_t **) buffer, bufferSize);
    // conver the audio from unsigned to signed
    for (int i = 0; i < audioLength; i++) {
      (*buffer)[i] = ((uint8_t)(*buffer)[i]) - 128;
    }
    return audioLength;
  }
  return 0;
}

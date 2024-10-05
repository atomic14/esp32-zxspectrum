#pragma once
#include "Screen.h"
#include "JPEGDEC.h"
#include "VideoPlayer/VideoPlayerState.h"

class VideoSource;

class VideoPlayer : public Screen
{
private:
  VideoPlayerState mState = VideoPlayerState::STOPPED;

  // video playing
  JPEGDEC mJpeg;

  // video source
  VideoSource *mVideoSource = NULL;

  // audio playing
  int mCurrentAudioSample = 0;
  AudioOutput *mAudioOutput = NULL;

  static void _framePlayerTask(void *param);
  static void _audioPlayerTask(void *param);

  void framePlayerTask();
  void audioPlayerTask();

  friend int _doDraw(JPEGDRAW *pDraw);

  using BackCallback = std::function<void()>;
  BackCallback m_backCallback;

public:
  VideoPlayer(
      TFTDisplay &tft,
      AudioOutput *audioOutput,
      BackCallback backCallback) : Screen(tft, audioOutput), m_backCallback(backCallback)
  {
  }
  void start();
  void play();
  void stop();
  void pause();
  void playStatic();
};
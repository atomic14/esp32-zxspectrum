#pragma once
#include "Screen.h"
// these two lines fix some weird compilation error with JPEGDEC
#include <FS.h>
using fs::File;
#include "JPEGDEC.h"
#include "VideoPlayer/VideoPlayerState.h"

class VideoSource;

class VideoPlayerScreen : public Screen
{
private:
  bool isStarted = false;
  std::string m_aviFilename;

  VideoPlayerState mState = VideoPlayerState::STOPPED;

  // video playing
  JPEGDEC mJpeg;

  // video source
  VideoSource *mVideoSource = NULL;

  // audio playing
  int mCurrentAudioSample = 0;

  static void _framePlayerTask(void *param);
  static void _audioPlayerTask(void *param);

  void framePlayerTask();
  void audioPlayerTask();

  friend int _doDraw(JPEGDRAW *pDraw);

  using BackCallback = std::function<void()>;
  BackCallback m_backCallback;

  void start();
  void stop();
  void pause();
  void playStatic();
public:
  VideoPlayerScreen(
      TFTDisplay &tft,
      AudioOutput *audioOutput,
      BackCallback backCallback) : Screen(tft, audioOutput), m_backCallback(backCallback)
  {
  }
  virtual void updatekey(SpecKeys key, uint8_t state) {
    stop();
    m_backCallback();
  }
  void play(const char *aviFilename);
};
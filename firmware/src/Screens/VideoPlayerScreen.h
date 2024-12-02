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
  JPEGDEC *mJpeg = new JPEGDEC();

  // video source
  VideoSource *mVideoSource = NULL;

  // audio playing
  int mCurrentAudioSample = 0;

  static void _framePlayerTask(void *param);
  static void _audioPlayerTask(void *param);

  void framePlayerTask();
  void audioPlayerTask();

  friend int _doDraw(JPEGDRAW *pDraw);

  void start();
  void stop();
  void pause();
  void playStatic();

  // volume controls
  int volumeVisible = 0;
public:
  VideoPlayerScreen(
      Display &tft,
      HDMIDisplay *hdmiDisplay,
      AudioOutput *audioOutput) : Screen(tft, hdmiDisplay, audioOutput)
  {
  }
  void play(const char *aviFilename);
  void pressKey(SpecKeys key);
};
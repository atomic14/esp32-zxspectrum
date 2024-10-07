#include "VideoPlayerScreen.h"
#include "VideoPlayer/VideoSource.h"
#include "AudioOutput/AudioOutput.h"
#include "../TFT/TFTDisplay.h"
#include <list>

void VideoPlayerScreen::_framePlayerTask(void *param)
{
  VideoPlayerScreen *player = (VideoPlayerScreen *)param;
  player->framePlayerTask();
}

void VideoPlayerScreen::_audioPlayerTask(void *param)
{
  VideoPlayerScreen *player = (VideoPlayerScreen *)param;
  player->audioPlayerTask();
}

void VideoPlayerScreen::start()
{
  if (isStarted)
  {
    return;
  }
  mState = VideoPlayerState::STOPPED;
  xTaskCreatePinnedToCore(
      _framePlayerTask,
      "Frame Player",
      10000,
      this,
      1,
      NULL,
      0);
  xTaskCreatePinnedToCore(_audioPlayerTask, "audio_loop", 10000, this, 1, NULL, 0);
  isStarted = true;
}

void VideoPlayerScreen::play(const char *aviFilename)
{
  start();
  if (mState != VideoPlayerState::STOPPED)
  {
    return;
  }
  if (mVideoSource != NULL)
  {
    delete mVideoSource;
  }
  m_aviFilename = aviFilename;
  mVideoSource = new VideoSource(aviFilename);
  mVideoSource->setState(VideoPlayerState::PLAYING);
  mState = VideoPlayerState::PLAYING;
  mCurrentAudioSample = 0;
}

void VideoPlayerScreen::stop()
{
  if (mState == VideoPlayerState::STOPPED)
  {
    return;
  }
  mState = VideoPlayerState::STOPPED;
  mVideoSource->setState(VideoPlayerState::STOPPED);
  mCurrentAudioSample = 0;
  m_tft.fillScreen(TFT_BLACK);
}

void VideoPlayerScreen::pause()
{
  if (mState == VideoPlayerState::PAUSED)
  {
    return;
  }
  mState = VideoPlayerState::PAUSED;
  mVideoSource->setState(VideoPlayerState::PAUSED);
}

void VideoPlayerScreen::playStatic()
{
  if (mState == VideoPlayerState::STATIC)
  {
    return;
  }
  mState = VideoPlayerState::STATIC;
  mVideoSource->setState(VideoPlayerState::STATIC);
}


// double buffer the dma drawing otherwise we get corruption
uint16_t *dmaBuffer[2] = {NULL, NULL};
int dmaBufferIndex = 0;
int _doDraw(JPEGDRAW *pDraw)
{
  VideoPlayerScreen *player = (VideoPlayerScreen *)pDraw->pUser;
  player->m_tft.setWindow(pDraw->x, pDraw->y, pDraw->x + pDraw->iWidth - 1, pDraw->y + pDraw->iHeight - 1);
  player->m_tft.pushPixels(pDraw->pPixels, pDraw->iWidth * pDraw->iHeight);
  return 1;
}

static unsigned short x = 12345, y = 6789, z = 42, w = 1729;

unsigned short xorshift16()
{
  unsigned short t = x ^ (x << 5);
  x = y;
  y = z;
  z = w;
  w = w ^ (w >> 1) ^ t ^ (t >> 3);
  return w & 0xFFFF;
}

void VideoPlayerScreen::framePlayerTask()
{
  uint16_t *staticBuffer = NULL;
  uint8_t *jpegBuffer = NULL;
  size_t jpegBufferLength = 0;
  size_t jpegLength = 0;
  // used for calculating frame rate
  std::list<int> frameTimes;
  while (true)
  {
    if (mState == VideoPlayerState::STOPPED || mState == VideoPlayerState::PAUSED)
    {
      // nothing to do - just wait
      vTaskDelay(100 / portTICK_PERIOD_MS);
      continue;
    }
    if (mState == VideoPlayerState::STATIC)
    {
      // draw random pixels to the screen to simulate static
      // we'll do this 8 rows of pixels at a time to save RAM
      int width = m_tft.width();
      int height = 8;
      if (staticBuffer == NULL)
      {
        staticBuffer = (uint16_t *)malloc(width * height * 2);
      }
      m_tft.startWrite();
      for (int i = 0; i < m_tft.height(); i++)
      {
        for (int p = 0; p < width * height; p++)
        {
          int grey = xorshift16() >> 8;
          staticBuffer[p] = m_tft.color565(grey, grey, grey);
        }
        m_tft.setWindow(0, i * height, width - 1, i * height + height - 1);
        m_tft.pushPixels(staticBuffer, width * height);
      }
      m_tft.endWrite();
      vTaskDelay(50 / portTICK_PERIOD_MS);
      continue;
    }
    // get the next frame
    if (!mVideoSource->getVideoFrame(&jpegBuffer, jpegBufferLength, jpegLength))
    {
      // no frame ready yet
      vTaskDelay(10 / portTICK_PERIOD_MS);
      continue;
    }
    frameTimes.push_back(millis());
    m_tft.startWrite();
    if (mJpeg.openRAM(jpegBuffer, jpegLength, _doDraw))
    {
      mJpeg.setUserPointer(this);
      #ifdef LED_MATRIX
      mJpeg.setPixelType(RGB565_LITTLE_ENDIAN);
      #else
      mJpeg.setPixelType(RGB565_BIG_ENDIAN);
      #endif
      mJpeg.decode(0, 0, 0);
      mJpeg.close();
    }
    #if CORE_DEBUG_LEVEL > 0
    m_tft.drawFPS(frameTimes.size() / 5);
    #endif
    m_tft.endWrite();
  }
}

void VideoPlayerScreen::audioPlayerTask()
{
  size_t bufferLength = 16000;
  uint8_t *audioData = (uint8_t *)malloc(16000);
  if (!audioData)
  {
    Serial.println("Failed to allocate audio buffer");
  }
  while (true)
  {
    if (mState != VideoPlayerState::PLAYING)
    {
      // nothing to do - just wait
      vTaskDelay(100 / portTICK_PERIOD_MS);
      continue;
    }
    // get audio data to play
    int audioLength = mVideoSource->getAudioSamples(&audioData, bufferLength, mCurrentAudioSample);
    // have we reached the end of the channel?
    if (audioLength == 0) {
      // we want to loop the video so reset the channel data and start again
      stop();
      play(m_aviFilename.c_str());
      continue;
    }
    if (audioLength > 0) {
      // play the audio
      for(int i=0; i<audioLength; i+=1000) {
        m_audioOutput->write(audioData + i, min(1000, audioLength - i));
        mCurrentAudioSample += min(1000, audioLength - i);
        if (mState != VideoPlayerState::PLAYING)
        {
          mCurrentAudioSample = 0;
          mVideoSource->updateAudioTime(0);
          break;
        }
        mVideoSource->updateAudioTime(1000 * mCurrentAudioSample / 16000);
        // vTaskDelay(1);
      }
    }
    else
    {
      vTaskDelay(100 / portTICK_PERIOD_MS);
    }
  }
}
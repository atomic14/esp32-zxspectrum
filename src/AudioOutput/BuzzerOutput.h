#pragma once
#include <freertos/FreeRTOS.h>
#include "AudioOutput.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "driver/sigmadelta.h"
#include "driver/timer.h"

/**
 * Base Class for both the ADC and I2S sampler
 **/
class BuzzerOutput : public AudioOutput
{
private:
  gpio_num_t mBuzzerPin;
  uint32_t mSampleRate;
  SemaphoreHandle_t mBufferSemaphore;
  int8_t *mBuffer=NULL;;
  int mCurrentIndex=0;
  int mBufferLength=0;
  int8_t *mSecondBuffer=NULL;;
  int mSecondBufferLength=0;
  int mCount = 0;
public:
  BuzzerOutput(gpio_num_t buzzerPin) : AudioOutput()
  {
    mBuzzerPin = buzzerPin;
    mBufferSemaphore = xSemaphoreCreateBinary();
    xSemaphoreGive(mBufferSemaphore);
  }
  void write(int8_t *samples, int count);
  void start(uint32_t sample_rate);
  void stop() {}

  bool onTimer();
};

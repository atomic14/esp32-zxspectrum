#pragma once
#include <freertos/FreeRTOS.h>
#include "AudioOutput.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "driver/sigmadelta.h"
#include "driver/timer.h"
#include <driver/adc.h>


class BuzzerOutput : public AudioOutput
{
private:
  gpio_num_t mBuzzerPin;
  uint32_t mSampleRate;
  SemaphoreHandle_t mBufferSemaphore;
  uint8_t *mBuffer=NULL;
  int mCurrentIndex=0;
  int mBufferLength=0;
  uint8_t *mSecondBuffer=NULL;
  int mSecondBufferLength=0;
  int mCount = 0;
  int micAve = 2048;
  // queue to hold samples read from the ADC
  xQueueHandle micValueQueue;
public:
  BuzzerOutput(gpio_num_t buzzerPin) : AudioOutput()
  {
    // set up the ADC
    // adc1_config_width(ADC_WIDTH_BIT_12);
    // adc1_config_channel_atten(ADC1_CHANNEL_4, ADC_ATTEN_DB_11);
    mBuzzerPin = buzzerPin;
    mBufferSemaphore = xSemaphoreCreateBinary();
    xSemaphoreGive(mBufferSemaphore);
    // create a queue to hold the values read from the ADC
    micValueQueue = xQueueCreate(312*2, sizeof(uint8_t));
  }
  void write(const uint8_t *samples, int count);
  bool getMicValue()
  {
    uint8_t value;
    if (xQueueReceive(micValueQueue, &value, 0)) {
      return value != 0;
    }
    return false;
  }
  void start(uint32_t sample_rate);
  void stop() {}
  void pause();
  void resume();
  bool onTimer();
};

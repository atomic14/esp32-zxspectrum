#include "BuzzerOutput.h"
#include "freertos/semphr.h"
#include "driver/sigmadelta.h"
#include <string.h>
#include <Arduino.h>

bool IRAM_ATTR onTimerCallback(void *args) {
  BuzzerOutput *output = (BuzzerOutput *)args;
  return output->onTimer();
  return false;
}

void BuzzerOutput::start(uint32_t sample_rate)
{
  mSampleRate = sample_rate;
  // pinMode(mBuzzerPin, OUTPUT);
  ledcSetup(0, 50000, 9);
  ledcAttachPin(mBuzzerPin, 0);
  // create a timer that will fire at the sample rate
  timer_config_t timer_config = {
      .alarm_en = TIMER_ALARM_EN,
      .counter_en = TIMER_PAUSE,
      .intr_type = TIMER_INTR_LEVEL,
      .counter_dir = TIMER_COUNT_UP,
      .auto_reload = TIMER_AUTORELOAD_EN,
      .divider = 80};
  ESP_ERROR_CHECK(timer_init(TIMER_GROUP_0, TIMER_0, &timer_config));
  ESP_ERROR_CHECK(timer_set_counter_value(TIMER_GROUP_0, TIMER_0, 0));
  ESP_ERROR_CHECK(timer_set_alarm_value(TIMER_GROUP_0, TIMER_0, 1000000 / sample_rate));
  ESP_ERROR_CHECK(timer_enable_intr(TIMER_GROUP_0, TIMER_0));
  ESP_ERROR_CHECK(timer_isr_callback_add(TIMER_GROUP_0, TIMER_0, onTimerCallback, this, 0));
  ESP_ERROR_CHECK(timer_start(TIMER_GROUP_0, TIMER_0));
}

void BuzzerOutput::pause()
{
  ledcWrite(0, 0);
  timer_pause(TIMER_GROUP_0, TIMER_0);
}

void BuzzerOutput::resume()
{
  timer_start(TIMER_GROUP_0, TIMER_0);
}


void BuzzerOutput::write(const uint8_t *samples, int count)
{
  while (true)
  {
    if(xSemaphoreTake(mBufferSemaphore, portMAX_DELAY)) {
      // is the second buffer empty?
      if (mSecondBufferLength == 0)
      {
        //Serial.println("Filling second buffer");
        // make sure there's enough room for the samples
        mSecondBuffer = (uint8_t *)realloc(mSecondBuffer, count);
        // copy them into the second buffer
        memcpy(mSecondBuffer, samples, count);
        // second buffer is now full of samples
        mSecondBufferLength = count;
        // unlock the mutext and return
        xSemaphoreGive(mBufferSemaphore);
        return;
      }
      // no room in the second buffer so wait for the first buffer to be emptied
      xSemaphoreGive(mBufferSemaphore);
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

bool IRAM_ATTR BuzzerOutput::onTimer()
{
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  // output a sample from the buffer if we have one
  if (mCurrentIndex < mBufferLength)
  {
    // uint16_t micSample = adc1_get_raw(ADC1_CHANNEL_4);
    // micAve = (micAve * 99 + micSample) / 100;
    // uint8_t micValue = micSample > (micAve * 1001)/1000 ? 1 : 0;
    uint8_t micValue = 0;
    xQueueSendFromISR(micValueQueue, &micValue, &xHigherPriorityTaskWoken);
    mCount++;
    // get the first sample from the buffer - shift it up to 9 bits for max resolution
    uint16_t sample = mBuffer[mCurrentIndex];
    if (micValue)
    {
      sample = std::min(255, sample + 50);
    }
    // go up to 9 bits resolution
    sample = sample << 1;
    sample = sample * mVolume / 10;
    // limit to 70% of the max volume - our speaker is 8ohm, 2W
    // 5 volts * 0.7 = 3.5 volts
    // P = V^2 / R
    // 3.5^2 / 8 = 1.2W
    sample = sample * 70 / 100;
    mCurrentIndex++;
    ledcWrite(0, sample);
  }
  if(mCurrentIndex >= mBufferLength)
  {
    // do we have any data in teh second buffer?
    if (xSemaphoreTakeFromISR(mBufferSemaphore, &xHigherPriorityTaskWoken) == pdTRUE)
    {
      if (mSecondBufferLength > 0) {
        // swap the buffers
        uint8_t *tmp = mBuffer;
        mBuffer = mSecondBuffer;
        mBufferLength = mSecondBufferLength;
        mSecondBuffer = tmp;
        mSecondBufferLength = 0;
        mCurrentIndex = 0;
      }
      xSemaphoreGiveFromISR(mBufferSemaphore, &xHigherPriorityTaskWoken);
    }
    return xHigherPriorityTaskWoken;
  }
  return false;
}
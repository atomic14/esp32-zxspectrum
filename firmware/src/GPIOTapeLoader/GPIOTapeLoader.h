#pragma once

#include <freertos/FreeRTOS.h>
#include <driver/adc.h>
#include <driver/timer.h>


IRAM_ATTR bool onTimer(void *param);
void launchTimer(void *param);

enum GPIOTapeLoaderState {
  STARTING,
  RUNNING,
  PAUSING,
  STOPPED
};

class GPIOTapeLoader
{
  private:
    // queue to hold samples read from the ADC
    xQueueHandle sampleQueue;
    friend IRAM_ATTR bool onTimer(void *param);
    friend void launchTimer(void *param);
    int sampleRate;
    GPIOTapeLoaderState state = STOPPED;
  public:
    GPIOTapeLoader(uint32_t sample_rate) {
      sampleRate = sample_rate;
      // create a queue to hold the values read from the ADC
      sampleQueue = xQueueCreate(16384, sizeof(uint16_t));
      xTaskCreatePinnedToCore(launchTimer, "launchTimer", 8192, this, 1, NULL, 0);
    }
    void start() {
      if (state == STARTING) return;
      state = STARTING;
    }
    void stop() {
      if (state == PAUSING) return;
      state = PAUSING;
    }
    void pause() {
      if (state == PAUSING) return;
      state = PAUSING;
    }
    void resume() {
      if (state == STARTING) return;
      state = STARTING;
    }
    uint16_t readSample() {
      uint16_t sample;
      if (xQueueReceive(sampleQueue, &sample, portMAX_DELAY)) {
        return sample;
      }
      return -1;
    }
};
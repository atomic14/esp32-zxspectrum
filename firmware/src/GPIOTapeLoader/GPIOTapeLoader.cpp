#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <driver/adc.h>
#include <driver/timer.h>
#include "GPIOTapeLoader.h"

#define TAPE_TIMER_GROUP TIMER_GROUP_1

IRAM_ATTR bool onTimer(void *param) {
  GPIOTapeLoader *loader = (GPIOTapeLoader *)param;
  // read from the ADC
  uint16_t sample = adc1_get_raw(ADC1_CHANNEL_4);

  if (sample > 2048) {
    ledcWrite(0, 10);
  } else {
    ledcWrite(0, 0);
  }
  // uint16_t sample = digitalRead(5) == LOW ? 4095 : 0;
  // push the sample on the queue
  BaseType_t higherPriorityTaskWoken = pdFALSE;
  xQueueSendFromISR(loader->sampleQueue, &sample, &higherPriorityTaskWoken);
  return higherPriorityTaskWoken == pdTRUE;
}

void launchTimer(void *param) {
  GPIOTapeLoader *loader = (GPIOTapeLoader *)param;
        // set up the ADC
      adc1_config_width(ADC_WIDTH_BIT_12);
      adc1_config_channel_atten(ADC1_CHANNEL_4, ADC_ATTEN_DB_12);
      // pinMode(5, INPUT);
      // create a timer that will fire at the sample rate
      timer_config_t timer_config = {
          .alarm_en = TIMER_ALARM_EN,
          .counter_en = TIMER_PAUSE,
          .intr_type = TIMER_INTR_LEVEL,
          .counter_dir = TIMER_COUNT_UP,
          .auto_reload = TIMER_AUTORELOAD_EN,
          .divider = 80,
          .clk_src = TIMER_SRC_CLK_APB
      };
      ESP_ERROR_CHECK(timer_init(TAPE_TIMER_GROUP, TIMER_0, &timer_config));
      ESP_ERROR_CHECK(timer_set_counter_value(TAPE_TIMER_GROUP, TIMER_0, 0));
      ESP_ERROR_CHECK(timer_set_alarm_value(TAPE_TIMER_GROUP, TIMER_0, 1000000 / loader->sampleRate));
      ESP_ERROR_CHECK(timer_enable_intr(TAPE_TIMER_GROUP, TIMER_0));
      ESP_ERROR_CHECK(timer_isr_callback_add(TAPE_TIMER_GROUP, TIMER_0, onTimer, loader, ESP_INTR_FLAG_IRAM));
      ESP_ERROR_CHECK(timer_start(TAPE_TIMER_GROUP, TIMER_0));
      while(1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        if (loader->state == GPIOTapeLoaderState::PAUSING) {
          timer_pause(TAPE_TIMER_GROUP, TIMER_0);
          loader->state = GPIOTapeLoaderState::STOPPED;
        } else if (loader->state == GPIOTapeLoaderState::STARTING) {
          timer_start(TAPE_TIMER_GROUP, TIMER_0);
          loader->state = GPIOTapeLoaderState::RUNNING;
        }
      }
}
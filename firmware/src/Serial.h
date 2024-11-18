#pragma once
// device only code
#ifdef ARDUINO_ARCH_ESP32

#include <Arduino.h>
#include "USB.h"
#include "USBMSC.h"

#ifdef NO_GLOBAL_SERIAL
extern USBCDC Serial;
extern USBMSC msc;
#endif

class SDCard;

void setupUSB(SDCard *_card);
void startMSC();

#define get_usecs esp_timer_get_time

#else

#include <stdio.h>
#include <stdarg.h>
#include <chrono>

class Logger {
  public:
  void println(const char *msg) {
    printf("%s\n", msg);
  }
  void printf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
  }
};

extern Logger Serial;

/*
{
    auto now = std::chrono::high_resolution_clock::now();
    auto micros = std::chrono::time_point_cast<std::chrono::microseconds>(now);
    return micros.time_since_epoch().count();
}
*/
#define get_usecs() std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now()).time_since_epoch().count()

#endif
#include <Arduino.h>
#include "Transport.h"

class SerialTransport : public Transport
{
public:
  SerialTransport() {
    // Serial.setTimeout(1);
    // Serial.setTxTimeoutMs(1);
    // Serial.setTxBufferSize(4096);
    // Serial.setRxBufferSize(4096);
  }

  bool available() override {
    return Serial.available() > 0;
  }

  int read() override {
    return Serial.read();
  }
  void write(uint8_t data) override {
    Serial.write(data);
  }
  void flush() {
    // Serial.flush();
  }
};

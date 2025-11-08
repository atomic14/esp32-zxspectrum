#include <Arduino.h>
#include "Transport.h"

class SerialTransport : public Transport
{
public:
  SerialTransport() {
    Serial.setTimeout(250);
    Serial.setTxBufferSize(60000);
    Serial.setRxBufferSize(60000);
  }

  bool available() override {
    return Serial.available() > 0;
  }

  int read() override {
    return Serial.read();
  }
  int read(uint8_t *data, int length){
    return Serial.read(data, length);
  }
  void write(uint8_t data) override {
    Serial.write(data);
  }
  void write(uint8_t *data, uint16_t length) override {
    Serial.write(data, length);
  }
  void flush() {
    Serial.flush();
  }
};

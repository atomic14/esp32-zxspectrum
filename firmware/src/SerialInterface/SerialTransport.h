#include "PacketHandler.h"

class SerialTransport : public Transport
{
public:
  SerialTransport(Serial &serial);

  int read() override {
    return serial.read();
  }
  void write(uint8_t data) override {
    serial.write(data);
  }

private:
  Serial &serial;
};
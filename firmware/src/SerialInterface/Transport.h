#pragma once
#include <stdint.h>

// Basic interface for a transport layer
class Transport
{
public:
  // is there any data to read?
  virtual bool available() = 0;
  // read a byte
  virtual int read() = 0;
  // read multiple bytes
  virtual int read(uint8_t *data, int length) = 0;
  // write a byte
  virtual void write(uint8_t data) = 0;
  // write multiple bytes
  virtual void write(uint8_t *data, uint16_t length) = 0;
  // flush
  virtual void flush() = 0;
};
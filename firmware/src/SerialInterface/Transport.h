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
  // write a byte
  virtual void write(uint8_t data) = 0;
  // flush
  virtual void flush() = 0;
};
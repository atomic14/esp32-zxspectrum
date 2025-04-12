#pragma once
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

enum class MessageId
{
  NullResponse = 0x00, // indicates no response is expected
  GetVersionRequest = 0x01,
  GetVersionResponse = 0x03,
  ListFolderRequest = 0x02,
  ListFolderResponse = 0x04,
  WriteFileRequest = 0x05,
  WriteFileResponse = 0x06,
  ReadFileRequest = 0x07,
  ReadFileResponse = 0x08,
  DeleteFileRequest = 0x09,
  DeleteFileResponse = 0x0A,
  MakeDirectoryRequest = 0x0B,
  MakeDirectoryResponse = 0x0C,
  RenameFileRequest = 0x0D,
  RenameFileResponse = 0x0E,
  GetFileInfoRequest = 0x0F,
  GetFileInfoResponse = 0x10,
};

class PacketHandler;
class MessageReciever
{
protected:
  PacketHandler *packetHandler = nullptr;

public:
  MessageReciever(PacketHandler *packetHandler) : packetHandler(packetHandler) {};
  virtual ~MessageReciever() {};
  virtual void messageStart(size_t dataLength) = 0;
  virtual void messageData(uint8_t *data, size_t length) = 0;
  virtual void messageFinished(bool isValid) = 0;

  virtual void sendSuccess(MessageId type);
  virtual void sendFailure(MessageId type, const char *errorMessage);
};

// This is a simple message reciever that doesn't need expect any data
class SimpleMessageReciever : public MessageReciever
{
public:
  SimpleMessageReciever(PacketHandler *packetHandler) : MessageReciever(packetHandler) {}
  void messageStart(size_t dataLength) override {}
  void messageData(uint8_t *data, size_t length) override {}
};

// This is a message receiver that accumulates the data in memory
class MemoryMessageReciever : public MessageReciever
{
private:
  uint8_t *buffer = nullptr;
  size_t bufferSize = 0;
  size_t offset = 0;

public:
  MemoryMessageReciever(PacketHandler *packetHandler) : MessageReciever(packetHandler) {};
  ~MemoryMessageReciever()
  {
    delete[] buffer;
  }
  void messageStart(size_t dataLength) override
  {
    delete[] buffer;
    buffer = new uint8_t[dataLength];
    bufferSize = dataLength;
    offset = 0;
  }
  void messageData(uint8_t *data, size_t length) override
  {
    if (offset + length <= bufferSize)
    {
      memcpy(buffer + offset, data, length);
      offset += length;
    }
  }
  uint8_t *getBuffer()
  {
    return buffer;
  }
  size_t getBufferSize()
  {
    return offset;
  }
};

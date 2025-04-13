#include "Message.h"
#include "../PacketHandler.h"
#include "../../Files/Files.h"

class WriteFileStartMessageReceiver : public MemoryMessageReciever
{
private:
  IFiles *flashFiles;
  IFiles *sdFiles;

public:
  WriteFileStartMessageReceiver(IFiles *flashFiles, IFiles *sdFiles, PacketHandler *packetHandler) 
  : MemoryMessageReciever(packetHandler),
    flashFiles(flashFiles),
    sdFiles(sdFiles)
  {};
  void messageFinished(bool isValid) override
  {
    if (isValid)
    {
      // filename is in the buffer - let's check that we can open it for writing
      const char *filename = (const char *)getBuffer();
      FILE *file = flashFiles->open(filename, "wb");
      if (file) {
        // respond with success
        sendSuccess(MessageId::WriteFileStartResponse);
        fclose(file);
      }
      else
      {
        // respond with failure
        sendFailure(MessageId::WriteFileStartResponse, "Could not open file");
      }
    }
    else
    {
      Serial.println("File write invalid");
      // respond with failure
      sendFailure(MessageId::WriteFileStartResponse, "Invalid message");
    }
  }
};


class WriteFileDataMessageReceiver : public MemoryMessageReciever
{
private:
  IFiles *flashFiles;
  IFiles *sdFiles;

public:
  WriteFileDataMessageReceiver(IFiles *flashFiles, IFiles *sdFiles, PacketHandler *packetHandler) 
  : MemoryMessageReciever(packetHandler),
    flashFiles(flashFiles),
    sdFiles(sdFiles)
  {};
  void messageFinished(bool isValid) override
  {
    if (isValid)
    {
      // filename is in the buffer as a null terminated string - open if for appending
      const char *filename = (const char *)getBuffer();
      FILE *file = flashFiles->open(filename, "ab");
      if (file) {
        auto bl = BusyLight();
        // write the data to the file - we need to skip past the filename and null terminating byte
        size_t dataLength = getBufferSize() - strlen(filename) - 1;
        size_t written = fwrite(getBuffer() + strlen(filename) + 1, 1, dataLength, file);
        if (written != dataLength)
        {
          // respond with failure
          sendFailure(MessageId::WriteFileDataResponse, "Could not write to file");
        } 
        else
        {          // respond with success
          sendSuccess(MessageId::WriteFileDataResponse);
        }
        fclose(file);
      }
      else
      {
        // respond with failure
        sendFailure(MessageId::WriteFileDataResponse, "Could not open file");
      }
    }
    else
    {
      Serial.println("File write invalid");
      // respond with failure
      sendFailure(MessageId::WriteFileDataResponse, "Invalid message");
    }
  }
};


class WriteFileEndMessageReceiver : public MemoryMessageReciever
{
private:
  IFiles *flashFiles;
  IFiles *sdFiles;

public:
  WriteFileEndMessageReceiver(IFiles *flashFiles, IFiles *sdFiles, PacketHandler *packetHandler) 
  : MemoryMessageReciever(packetHandler),
    flashFiles(flashFiles),
    sdFiles(sdFiles)
  {};
  void messageFinished(bool isValid) override
  {
    if (isValid)
    {
      // filename is in the buffer as a null terminated string followed by 4 bytes for the length of file
      // get the length of the file on disk
      const char *filename = (const char *)getBuffer();
      FILE *file = flashFiles->open(filename, "rb");
      if (file) {
        // get the length of the file
        fseek(file, 0, SEEK_END);
        size_t fileSize = ftell(file);
        fclose(file);
        // check the size matches the size in the message
        size_t expectedSize = *(size_t *)(getBuffer() + strlen(filename) + 1);
        if (fileSize != expectedSize)
        {
          // respond with failure
          char message[1000];
          snprintf(message, sizeof(message), "File size mismatch: %zu != %zu", fileSize, expectedSize);
          sendFailure(MessageId::WriteFileEndResponse, message);
          flashFiles->remove(filename);
          return;
        }
        // respond with success
        sendSuccess(MessageId::WriteFileEndResponse);
      }
      else
      {
        // respond with failure
        sendFailure(MessageId::WriteFileEndResponse, "Could not open file");
      }
    }
    else
    {
      Serial.println("File write invalid");
      // respond with failure
      sendFailure(MessageId::WriteFileEndResponse, "Invalid message");
      flashFiles->remove((const char *)getBuffer());
    }
  }
};
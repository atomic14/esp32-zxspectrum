#include "Message.h"
#include "../PacketHandler.h"
#include "../../Files/Files.h"

class WriteFileStartMessageReceiver : public MemoryMessageReciever
{
private:
  IFiles *flashFiles;
  IFiles *sdFiles;

public:
  WriteFileStartMessageReceiver(FilesImplementation<FlashLittleFS> *flashFiles, FilesImplementation<SDCard> *sdFiles, PacketHandler *packetHandler) 
  : MemoryMessageReciever(packetHandler),
    flashFiles(flashFiles),
    sdFiles(sdFiles)
  {};
  void messageFinished(bool isValid) override
  {
    if (isValid)
    {
      JsonDocument doc;
      auto error = deserializeJson(doc, getBuffer());
      if (error != DeserializationError::Ok)
      {
        sendFailure(MessageId::WriteFileStartResponse, "Invalid request");
        return;
      }
      const char *path = doc["path"];
      if (!path)
      {
        sendFailure(MessageId::WriteFileStartResponse, "Missing path");
        return;
      }
      bool isFlash = doc["isFlash"];
      FILE *file = isFlash ? flashFiles->open(path, "wb") : sdFiles->open(path, "wb");
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
  WriteFileDataMessageReceiver(FilesImplementation<FlashLittleFS> *flashFiles, FilesImplementation<SDCard> *sdFiles, PacketHandler *packetHandler) 
  : MemoryMessageReciever(packetHandler),
    flashFiles(flashFiles),
    sdFiles(sdFiles)
  {};
  void messageFinished(bool isValid) override
  {
    if (isValid)
    {
      // first part of the buffer is the json object - second part is the data
      const char *json = (const char *)getBuffer();
      uint8_t *data = getBuffer() + strlen(json) + 1;
      size_t dataLength = getBufferSize() - strlen(json) - 1;

      JsonDocument doc;
      auto error = deserializeJson(doc, json);
      if (error != DeserializationError::Ok)
      {
        sendFailure(MessageId::WriteFileDataResponse, "Invalid request");
        return;
      }
      const char *path = doc["path"];
      if (!path)
      {
        sendFailure(MessageId::WriteFileDataResponse, "Missing path");
        return;
      }
      bool isFlash = doc["isFlash"];
      FILE *file = isFlash ? flashFiles->open(path, "ab") : sdFiles->open(path, "ab");
      if (file) {
        auto bl = BusyLight();
        // write the data to the file - we need to skip past the filename and null terminating byte
        size_t written = fwrite(data, 1, dataLength, file);
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
  WriteFileEndMessageReceiver(FilesImplementation<FlashLittleFS> *flashFiles, FilesImplementation<SDCard> *sdFiles, PacketHandler *packetHandler) 
  : MemoryMessageReciever(packetHandler),
    flashFiles(flashFiles),
    sdFiles(sdFiles)
  {};
  void messageFinished(bool isValid) override
  {
    if (isValid)
    {
      JsonDocument doc;
      auto error = deserializeJson(doc, getBuffer());
      if (error != DeserializationError::Ok)
      {
        sendFailure(MessageId::WriteFileEndResponse, "Invalid JSON");
        return;
      }
      const char *path = doc["path"];
      if (!path)
      {
        sendFailure(MessageId::WriteFileEndResponse, "Missing path");
        return;
      }
      bool isFlash = doc["isFlash"];
      size_t expectedSize = doc["size"];
      if (expectedSize == 0)
      {
        sendFailure(MessageId::WriteFileEndResponse, "Missing size");
        return;
      }
      FILE *file = isFlash ? flashFiles->open(path, "rb") : sdFiles->open(path, "rb");
      if (file) {
        // get the length of the file
        fseek(file, 0, SEEK_END);
        size_t fileSize = ftell(file);
        fclose(file);
        if (fileSize != expectedSize)
        {
          // respond with failure
          char message[1000];
          snprintf(message, sizeof(message), "File size mismatch: %zu != %zu", fileSize, expectedSize);
          sendFailure(MessageId::WriteFileEndResponse, message);
          if (isFlash)
          {
            flashFiles->remove(path);
          }
          else
          {
            sdFiles->remove(path);
          }
          return;
        }
        // respond with success
        sendSuccess(MessageId::WriteFileEndResponse);
      }
      else
      {
        // create an error messgae with the path
        char message[1000];
        snprintf(message, sizeof(message), "Could not open file: %s", path);
        // respond with failure
        sendFailure(MessageId::WriteFileEndResponse, message);
        return;
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
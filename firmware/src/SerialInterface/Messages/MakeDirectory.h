#include "Message.h"
#include "../PacketHandler.h"
#include "../../Files/Files.h"

class MakeDirectoryMessageReceiver : public MemoryMessageReciever
{
private:
  IFiles *sdFiles = nullptr;
  IFiles *flashFiles = nullptr;
public:
  MakeDirectoryMessageReceiver(FilesImplementation<FlashLittleFS> *flashFiles, FilesImplementation<SDCard> *sdFiles, PacketHandler *packetHandler) : sdFiles(sdFiles), flashFiles(flashFiles), MemoryMessageReciever(packetHandler) {};
  void messageFinished(bool isValid) override
  {
    // filename will be in the buffer
    if (isValid)
    {
      JsonDocument doc;
      auto error = deserializeJson(doc, getBuffer());
      if (error != DeserializationError::Ok)
      {
        sendFailure(MessageId::MakeDirectoryResponse, "Invalid JSON");
        return;
      }
      const char *path = doc["path"];
      if (!path)
      {
        sendFailure(MessageId::MakeDirectoryResponse, "Missing path");
        return;
      }
      bool isFlash = doc["isFlash"];
      bool success = false;
      if (isFlash)
      {
        success = flashFiles->createDirectory(path);
      }
      else
      {
        success = sdFiles->createDirectory(path);
      }
      if (success)
      {
        sendSuccess(MessageId::MakeDirectoryResponse);
      }
      else
      {
        sendFailure(MessageId::MakeDirectoryResponse, "Failed to create directory");
      }
    }
    else
    {
      sendFailure(MessageId::MakeDirectoryResponse, "Invalid request");
    }
  }
};

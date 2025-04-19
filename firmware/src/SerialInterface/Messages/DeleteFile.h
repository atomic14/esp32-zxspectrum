#include "Message.h"
#include <ArduinoJson.h>
#include "../PacketHandler.h"
#include "../../Files/Files.h"

class DeleteFileMessageReceiver : public MemoryMessageReciever
{
private:
  IFiles *sdFiles = nullptr;
  IFiles *flashFiles = nullptr;
public:
  DeleteFileMessageReceiver(FilesImplementation<FlashLittleFS> *flashFiles, FilesImplementation<SDCard> *sdFiles, PacketHandler *packetHandler) : sdFiles(sdFiles), flashFiles(flashFiles), MemoryMessageReciever(packetHandler) {};
  void messageFinished(bool isValid) override
  {
    if (isValid)
    {
      JsonDocument doc;
      auto error = deserializeJson(doc, getBuffer());
      if (error != DeserializationError::Ok)
      {
        sendFailure(MessageId::DeleteFileResponse, "Invalid JSON");
        return;
      }
      const char *path = doc["path"];
      if (!path)
      {
        sendFailure(MessageId::DeleteFileResponse, "Missing path");
        return;
      }
      bool isFlash = doc["isFlash"];
      bool result = false;
      if (isFlash)
      {
        result = flashFiles->remove(path);
      }
      else
      {
        sdFiles->remove(path);
      }
      if (result)
      {
        sendSuccess(MessageId::DeleteFileResponse);
      }
      else
      {
        sendFailure(MessageId::DeleteFileResponse, "Failed to delete file/folder");
      }
    }
    else
    {
      sendFailure(MessageId::DeleteFileResponse, "Invalid request");
    }
  }
};

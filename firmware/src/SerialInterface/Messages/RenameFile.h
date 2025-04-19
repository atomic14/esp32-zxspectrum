#include "Message.h"
#include "../PacketHandler.h"
#include "../../Files/Files.h"

class RenameFileMessageReceiver : public MemoryMessageReciever
{
private:
  IFiles *sdFiles = nullptr;
  IFiles *flashFiles = nullptr;
public:
  RenameFileMessageReceiver(FilesImplementation<FlashLittleFS> *flashFiles, FilesImplementation<SDCard> *sdFiles, PacketHandler *packetHandler) : sdFiles(sdFiles), flashFiles(flashFiles), MemoryMessageReciever(packetHandler) {};
  void messageFinished(bool isValid) override
  {
    // filenames will be in the buffer
    if (isValid)
    {
      JsonDocument doc;
      auto error = deserializeJson(doc, getBuffer());
      if (error != DeserializationError::Ok)
      {
        sendFailure(MessageId::RenameFileResponse, "Invalid JSON");
        return;
      }
      const char *sourcePath = doc["sourcePath"];
      if (!sourcePath)
      {
        sendFailure(MessageId::RenameFileResponse, "Missing source path");
        return;
      }
      const char *destinationPath = doc["destinationPath"];
      if (!destinationPath)
      {
        sendFailure(MessageId::RenameFileResponse, "Missing destination path");
        return;
      }
      bool isFlash = doc["isFlash"];
      // rename the file
      bool success = false;
      if (isFlash)
      {
        success = flashFiles->rename(sourcePath, destinationPath);
      }
      else
      {
        success = sdFiles->rename(sourcePath, destinationPath);
      }
      if (success)
      {
        sendSuccess(MessageId::RenameFileResponse);
      }
      else
      {
        sendFailure(MessageId::RenameFileResponse, "Failed to rename file");
      }
    }
    else 
    {
      // send failure response
      sendFailure(MessageId::RenameFileResponse, "Invalid request");
    }
  }
};

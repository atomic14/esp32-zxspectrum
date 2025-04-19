#include "Message.h"
#include "../PacketHandler.h"
#include "../../Files/Files.h"

class ListFolderMessageReceiver : public MemoryMessageReciever
{
  private:
    IFiles *sdFiles = nullptr;
    IFiles *flashFiles = nullptr;
  public:
    ListFolderMessageReceiver(FilesImplementation<FlashLittleFS> *flashFiles, FilesImplementation<SDCard> *sdFiles, PacketHandler *packetHandler) : sdFiles(sdFiles), flashFiles(flashFiles), MemoryMessageReciever(packetHandler) {};

    void messageFinished(bool isValid) override
    {
      if (isValid)
      {
        JsonDocument doc;
        auto error = deserializeJson(doc, getBuffer());
        if (error != DeserializationError::Ok)
        {
          sendFailure(MessageId::ListFolderResponse, "Invalid JSON");
          return;
        }
        const char *path = doc["path"];
        if (!path)
        {
          sendFailure(MessageId::ListFolderResponse, "Missing path");
          return;
        }
        bool isFlash = doc["isFlash"];
        FileInfoVector filesVector;
        if (isFlash)
        {
          filesVector = flashFiles->getFileStartingWithPrefix(path, nullptr, {}, true);
        }
        else
        {
          filesVector = sdFiles->getFileStartingWithPrefix(path, nullptr, {}, true);
        }

        ArduinoJson::JsonDocument responseDoc;
        responseDoc["success"] = true;
        auto filesResult = responseDoc["result"]["files"].to<JsonArray>();

        for (auto file : filesVector)
        {
          ArduinoJson::JsonObject fileObject = filesResult.add<JsonObject>();
          fileObject["name"] = file->getName();
          fileObject["isDirectory"] = file->isDirectory();
        }
        sendSuccess(MessageId::ListFolderResponse, responseDoc);
      }
      else
      {
        sendFailure(MessageId::ListFolderResponse, "Invalid request");
      }
    }
};

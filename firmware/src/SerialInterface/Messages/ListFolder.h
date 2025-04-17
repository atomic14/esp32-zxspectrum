#include "Message.h"
#include "../PacketHandler.h"
#include "../../Files/Files.h"

class ListFolderMessageReceiver : public MemoryMessageReciever
{
  private:
    IFiles *files = nullptr;
  public:
    ListFolderMessageReceiver(IFiles *files, PacketHandler *packetHandler) : files(files), MemoryMessageReciever(packetHandler) {};

    void messageFinished(bool isValid) override
    {
      if (isValid)
      {
        // the buffer should contain the null terminated path to the folder
        Serial.printf("List folder: %s\n", getBuffer());

        FileInfoVector filesVector = files->getFileStartingWithPrefix((const char *) getBuffer(), nullptr, {}, true);

        ArduinoJson::JsonDocument doc;
        doc["success"] = true;
        auto filesResult = doc["result"]["files"].to<JsonArray>();

        for (auto file : filesVector)
        {
          ArduinoJson::JsonObject fileObject = filesResult.add<JsonObject>();
          fileObject["name"] = file->getName();
          fileObject["isDirectory"] = file->isDirectory();
        }
        std::stringstream response;
        serializeJson(doc, response);

        std::string responseString = response.str();
        size_t responseLength = responseString.length();

        packetHandler->sendPacket(MessageId::ListFolderResponse, (uint8_t *) responseString.c_str(), responseLength);
      }
    }
};

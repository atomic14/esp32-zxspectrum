#include <ArduinoJson.h>
#include "Message.h"
#include "../PacketHandler.h"
#include "version_info.h"

class GetVersionMessageReciever : public SimpleMessageReciever
{
  private:
    IFiles *flashFiles;
    IFiles *sdFiles;
  public:
    GetVersionMessageReciever(IFiles *flashFiles, IFiles *sdFiles, PacketHandler *packetHandler)
    : flashFiles(flashFiles), 
      sdFiles(sdFiles),
      SimpleMessageReciever(packetHandler)
    {
    }
    void messageFinished(bool isValid) override
    {
      if (isValid)
      {
        JsonDocument doc;
        doc["success"] = true;
        doc["result"]["firmwareVersion"] = FIRMWARE_VERSION_STRING;
        doc["result"]["hardwareVersion"] = HARDWARE_VERSION_STRING;
        auto flash = doc["result"]["flash"].as<JsonObject>();
        flash["available"] = flashFiles->isAvailable();
        uint64_t total = 0, used = 0;
        flashFiles->getSpace(total, used);
        flash["total"] = total;
        flash["used"] = used;
        auto sd = doc["result"]["sd"].as<JsonObject>();
        sd["available"] = sdFiles->isAvailable();
        if (sdFiles->isAvailable()) {
          sdFiles->getSpace(total, used);
          sd["total"] = total;
          sd["used"] = used;
        } else {
          sd["total"] = 0;
          sd["used"] = 0;
        }

        sendSuccess(MessageId::GetVersionResponse, doc);
      }
    }
};

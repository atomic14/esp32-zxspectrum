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
    GetVersionMessageReciever(FilesImplementation<FlashLittleFS> *flashFiles, FilesImplementation<SDCard> *sdFiles, PacketHandler *packetHandler)
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
        doc["result"]["flash"]["available"] = flashFiles->isAvailable();
        uint64_t total = 0, used = 0;
        flashFiles->getSpace(total, used);
        doc["result"]["flash"]["total"] = total;
        doc["result"]["flash"]["used"] = used;

        doc["result"]["sd"]["available"] = sdFiles->isAvailable();
        if (sdFiles->isAvailable()) {
          sdFiles->getSpace(total, used);
          doc["result"]["sd"]["total"] = total;
          doc["result"]["sd"]["used"] = used;
        } else {
          doc["result"]["sd"]["total"] = 0;
          doc["result"]["sd"]["used"] = 0;
        }

        sendSuccess(MessageId::GetVersionResponse, doc);
      }
    }
};

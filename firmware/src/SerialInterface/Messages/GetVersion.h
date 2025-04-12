#include <ArduinoJson.h>
#include "Message.h"
#include "../PacketHandler.h"

class GetVersionMessageReciever : public SimpleMessageReciever
{
  public:
    uint8_t major, minor, build;

    GetVersionMessageReciever(PacketHandler *packetHandler, uint8_t major, uint8_t minor, uint8_t build) : SimpleMessageReciever(packetHandler)
    {
      this->major = major;
      this->minor = minor;
      this->build = build;
    }
    void messageFinished(bool isValid) override
    {
      if (isValid)
      {
        JsonDocument doc;
        doc["success"] = true;
        doc["result"]["version"]["major"] = major;
        doc["result"]["version"]["minor"] = minor;
        doc["result"]["version"]["build"] = build;



        std::stringstream response;
        serializeJson(doc, response);

        std::string responseString = response.str();
        size_t responseLength = responseString.length();

        packetHandler->sendPacket(MessageId::GetVersionResponse, (uint8_t *) responseString.c_str(), responseLength);
      }
    }
};

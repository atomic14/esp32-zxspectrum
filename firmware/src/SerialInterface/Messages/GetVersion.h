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
        // respond with the version information
        uint8_t response[] = {
            major,
            minor,
            build};
        packetHandler->sendPacket(MessageId::GetVersionResponse, response, sizeof(response));
      }
    }
};

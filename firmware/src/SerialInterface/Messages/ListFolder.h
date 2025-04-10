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

        // loop through the files and create the response
        std::string response = "";
        for (auto file : filesVector)
        {
          if (response.length() > 0)
          {
            response += "|";
          }
          response += file->getPath();
          if (file->isDirectory())
          {
            response += "/";
          }
        }
        packetHandler->sendPacket(MessageId::ListFolderResponse, (uint8_t *) response.c_str(), response.size());
      }
    }
};

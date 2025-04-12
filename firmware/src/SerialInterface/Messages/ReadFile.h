#include "Message.h"
#include "../PacketHandler.h"
#include "../../Files/Files.h"

class ReadFileMessageReceiver : public MemoryMessageReciever
{
private:
  IFiles *files = nullptr;
public:
  ReadFileMessageReceiver(IFiles *files, PacketHandler *packetHandler) : files(files), MemoryMessageReciever(packetHandler) {};
  void messageFinished(bool isValid) override
  {
    // filename will be in the buffer
    if (isValid)
    {
      FILE *file = files->open((const char *) getBuffer(), "rb");
      if (file)
      {
        // read the file into the buffer
        fseek(file, 0, SEEK_END);
        size_t fileSize = ftell(file);
        fseek(file, 0, SEEK_SET);
        uint8_t *buffer = new uint8_t[fileSize];
        fread(buffer, 1, fileSize, file);
        fclose(file);
        file = nullptr;
        // respond with the file data
        packetHandler->sendPacket(MessageId::ReadFileResponse, buffer, fileSize);
        delete[] buffer;
      }
      else
      {
        // respond with failure
        packetHandler->sendPacket(MessageId::ReadFileResponse, nullptr, 0);
      }
    }
  }
};

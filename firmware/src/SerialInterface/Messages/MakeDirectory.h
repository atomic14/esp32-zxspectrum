#include "Message.h"
#include "../PacketHandler.h"
#include "../../Files/Files.h"

class MakeDirectoryMessageReceiver : public MemoryMessageReciever
{
private:
  IFiles *files = nullptr;
public:
  MakeDirectoryMessageReceiver(IFiles *files, PacketHandler *packetHandler) : files(files), MemoryMessageReciever(packetHandler) {};
  void messageFinished(bool isValid) override
  {
    // filename will be in the buffer
    if (isValid)
    {
      if (files->createDirectory((const char *) getBuffer())) {
        sendSuccess(MessageId::MakeDirectoryResponse);
      }
      else 
      {
        sendFailure(MessageId::MakeDirectoryResponse, "Failed to create directory");
      }
    }
    else
    {
      sendFailure(MessageId::MakeDirectoryResponse, "Invalid request");
    }
  }
};

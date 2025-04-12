#include "Message.h"
#include "../PacketHandler.h"
#include "../../Files/Files.h"

class DeleteFileMessageReceiver : public MemoryMessageReciever
{
private:
  IFiles *files = nullptr;
public:
  DeleteFileMessageReceiver(IFiles *files, PacketHandler *packetHandler) : files(files), MemoryMessageReciever(packetHandler) {};
  void messageFinished(bool isValid) override
  {
    // filename will be in the buffer
    if (isValid)
    {
      files->remove((const char *) getBuffer());
      sendSuccess(MessageId::DeleteFileResponse);
    }
    else
    {
      sendFailure(MessageId::DeleteFileResponse, "Invalid request");
    }
  }
};

#include "Message.h"
#include "../PacketHandler.h"
#include "../../Files/Files.h"

class RenameFileMessageReceiver : public MemoryMessageReciever
{
private:
  IFiles *files = nullptr;
public:
  RenameFileMessageReceiver(IFiles *files, PacketHandler *packetHandler) : files(files), MemoryMessageReciever(packetHandler) {};
  void messageFinished(bool isValid) override
  {
    // filenames will be in the buffer
    if (isValid)
    {
      // existing name will be the first null terminated string, new name will be the second null terminated string
      const char *buffer = (const char *) getBuffer();
      const char *oldFilename = buffer;
      const char *newFilename = buffer + strlen(oldFilename) + 1;
      // rename the file
      files->rename(oldFilename, newFilename);
      // send success response
      sendSuccess(MessageId::RenameFileResponse);
    }
    else 
    {
      // send failure response
      sendFailure(MessageId::RenameFileResponse, "Invalid request");
    }
  }
};

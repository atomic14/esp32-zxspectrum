#include "Message.h"
#include "../PacketHandler.h"
#include "../../Files/Files.h"

class WriteFileMessageReceiver : public MessageReciever
{
private:
  std::string filename;
  FILE *file = nullptr;
  bool readingFilename = true;

public:
  WriteFileMessageReceiver(PacketHandler *packetHandler) : MessageReciever(packetHandler) {};
  void messageStart(size_t dataLength) override
  {
    readingFilename = true;
  }

  void messageData(uint8_t *data, size_t length) override
  {
    // are we reading the filename?
    if (readingFilename)
    {
      // read until we hit a null byte
      for (size_t i = 0; i < length; i++)
      {
        if (data[i] == '\0')
        {
          readingFilename = false;
          file = fopen(filename.c_str(), "wb");
          // write the rest of the data to the file
          if (file)
          {
            fwrite(data + i + 1, 1, length - i - 1, file);
          }
          break;
        }
        else
        {
          filename += (char)data[i];
        }
      }
    }
    else
    {
      // write the data to the file
      if (file)
      {
        fwrite(data, 1, length, file);
      }
    }
  }

  void messageFinished(bool isValid) override
  {
    // close the file
    if (isValid)
    {
      if (file) {
        // respond with success
        packetHandler->sendPacket(MessageId::WriteFileResponse, (uint8_t *)"OK", 2);
        if (file)
        {
          fclose(file);
          file = nullptr;
        }
      }
      else
      {
        // respond with failure
        packetHandler->sendPacket(MessageId::WriteFileResponse, (uint8_t *)"FAIL_FILE", 9);
      }
    }
    else
    {
      // delete the file if it was created
      if (file)
      {
        fclose(file);
        file = nullptr;
        remove(filename.c_str());
      }
      // respond with failure
      packetHandler->sendPacket(MessageId::WriteFileResponse, (uint8_t *)"FAIL", 4);
      readingFilename = false;
    }
  }
};

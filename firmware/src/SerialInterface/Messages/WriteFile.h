#include "Message.h"
#include "../PacketHandler.h"
#include "../../Files/Files.h"

class WriteFileMessageReceiver : public MessageReciever
{
private:
  std::string filename;
  IFiles *flashFiles;
  IFiles *sdFiles;
  FILE *file = nullptr;
  bool readingFilename = true;

public:
  WriteFileMessageReceiver(IFiles *flashFiles, IFiles *sdFiles, PacketHandler *packetHandler) 
  : MessageReciever(packetHandler),
    flashFiles(flashFiles),
    sdFiles(sdFiles)
  {};
  void messageStart(size_t dataLength) override
  {
    Serial.printf("Message length is %d\n", dataLength);
    readingFilename = true;
    filename = "";
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
          file = flashFiles->open(filename.c_str(), "wb");
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
        sendSuccess(MessageId::WriteFileResponse);
        if (file)
        {
          fclose(file);
          file = nullptr;
        }
      }
      else
      {
        // respond with failure
        sendFailure(MessageId::WriteFileResponse, "Could not open file");
        readingFilename = false;
      }
    }
    else
    {
      Serial.println("File write invalid");
      // delete the file if it was created
      if (file)
      {
        fclose(file);
        file = nullptr;
        remove(filename.c_str());
      }
      // respond with failure
      sendFailure(MessageId::WriteFileResponse, "Invalid message");
      readingFilename = false;
    }
  }
};

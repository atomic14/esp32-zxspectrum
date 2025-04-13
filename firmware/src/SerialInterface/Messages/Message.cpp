#include <ArduinoJson.h>
#include <sstream>
#include "Message.h"
#include "../PacketHandler.h"

void MessageReciever::sendSuccess(MessageId type)
{
  ArduinoJson::JsonDocument doc;
  doc["success"] = true;
  std::stringstream ss;
  serializeJson(doc, ss);
  std::string jsonString = ss.str();
  size_t length = jsonString.length();
  packetHandler->sendPacket(type, (uint8_t *)jsonString.c_str(), length);
}

void MessageReciever::sendSuccess(MessageId type, ArduinoJson::JsonDocument &doc)
{
  std::stringstream ss;
  serializeJson(doc, ss);
  std::string jsonString = ss.str();
  size_t length = jsonString.length();
  packetHandler->sendPacket(type, (uint8_t *)jsonString.c_str(), length);
}

void MessageReciever::sendFailure(MessageId type, const char *errorMessage)
{
  ArduinoJson::JsonDocument doc;
  doc["success"] = false;
  doc["errorMessage"] = errorMessage;
  std::stringstream ss;
  serializeJson(doc, ss);
  std::string jsonString = ss.str();
  size_t length = jsonString.length();
  packetHandler->sendPacket(type, (uint8_t *)jsonString.c_str(), length);
}

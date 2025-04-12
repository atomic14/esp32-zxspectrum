#include <Arduino.h>
#include "LittleFS.h"
#include "FlashLittleFS.h"
#include "Serial.h"

FlashLittleFS::FlashLittleFS(const char *mountPoint)
{
  m_mountPoint = mountPoint;
  Serial.println("Initialising flash filesystem");
  if (LittleFS.begin(true, mountPoint))
  {
    Serial.println("Flash filesystem mounted successfully");
    _isMounted = true;
  } else
  {
    _isMounted = false;
    Serial.println("An error occurred while mounting SPIFFS");
  }
}

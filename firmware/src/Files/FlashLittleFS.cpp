#include <Arduino.h>
#include <esp_littlefs.h>
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
  }
  else
  {
    _isMounted = false;
    Serial.println("An error occurred while mounting SPIFFS");
  }
}

bool FlashLittleFS::getSpace(size_t &total, size_t &used)
{
  if (!_isMounted)
  {
    return false;
  }
  if (esp_littlefs_info("spiffs", &total, &used))
  {
    return 0;
  }
}
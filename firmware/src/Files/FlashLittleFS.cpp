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

bool FlashLittleFS::getSpace(uint64_t &total, uint64_t &used)
{
  if (!_isMounted)
  {
    return false;
  }
  
  // esp_littlefs_info uses size_t, so we need to use temporary variables
  size_t temp_total = 0, temp_used = 0;
  
  if (esp_littlefs_info("spiffs", &temp_total, &temp_used) == ESP_OK)
  {
    total = temp_total;
    used = temp_used;
    return true;
  }
  return false;
}
#include <Arduino.h>
#include <SPIFFS.h>
#include "FlashSPIFFS.h"
#include "Serial.h"

FlashSPIFFS::FlashSPIFFS(const char *mountPoint)
{
  m_mountPoint = mountPoint;
  Serial.println("Initialising flash filesystem");
  if (SPIFFS.begin(true, mountPoint))
  {
    Serial.println("Flash filesystem mounted successfully");
    _isMounted = true;
  } else
  {
    _isMounted = false;
    Serial.println("An error occurred while mounting SPIFFS");
  }
}

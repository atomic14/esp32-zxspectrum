#include <Arduino.h>
#include <SPIFFS.h>
#include "Flash.h"

Flash::Flash(const char *mountPoint)
{
  if (!SPIFFS.begin(true, mountPoint))
  {
    Serial.println("An error occurred while mounting SPIFFS");
  }
}

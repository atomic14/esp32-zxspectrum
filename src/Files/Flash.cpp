#include <Arduino.h>
#include <SPIFFS.h>
#include "Flash.h"

Flash::Flash()
{
  if (!SPIFFS.begin(true, MOUNT_POINT))
  {
    Serial.println("An error occurred while mounting SPIFFS");
  }
}

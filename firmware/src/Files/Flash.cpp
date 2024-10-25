#include <Arduino.h>
#include <SPIFFS.h>
#include "Flash.h"
#include "Serial.h"

Flash::Flash(const char *mountPoint)
{
  if (SPIFFS.begin(true, mountPoint))
  {
    _isMounted = true;
  } else
  {
    _isMounted = false;
    Serial.println("An error occurred while mounting SPIFFS");
  }
}

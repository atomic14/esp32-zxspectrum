#include <Arduino.h>
#include "USB.h"
#include "USBMSC.h"
#include "Serial.h"
#include "./Files/SDCard.h"
#include "./Files/Files.h"

USBMSC msc;
USBCDC Serial;

static SDCard *card;

static int32_t onWrite(uint32_t lba, uint32_t offset, uint8_t *buffer, uint32_t bufsize)
{
  auto bl = BusyLight();
  // Serial.printf("Writing %d bytes to %d at offset %d\n", bufsize, lba, offset);
  // this writes a complete sector so we should return sector size on success
  if (card->writeSectors(buffer, lba, bufsize / card->getSectorSize()))
  {
    return bufsize;
  }
  return bufsize;
  // return -1;
}

static int32_t onRead(uint32_t lba, uint32_t offset, void *buffer, uint32_t bufsize)
{
  auto bl = BusyLight();
  // Serial.printf("Reading %d bytes from %d at offset %d\n", bufsize, lba, offset);
  // this reads a complete sector so we should return sector size on success
  if (card->readSectors((uint8_t *)buffer, lba, bufsize / card->getSectorSize()))
  {
    return bufsize;
  }
  return -1;
}

static bool onStartStop(uint8_t power_condition, bool start, bool load_eject)
{
  Serial.printf("StartStop: %d %d %d\n", power_condition, start, load_eject);
  if (load_eject)
  {
    msc.end();
  }
  return true;
}

void setupUSB(SDCard *_card)
{
  if (_card) {
    card = _card;
  }
  Serial.begin(115200);
  USB.begin();
}

void startMSC()
{
  msc.vendorID("atomic14");
  msc.productID("ESP32Rainbow");
  msc.productRevision("1.0");
  msc.onRead(onRead);
  msc.onWrite(onWrite);
  msc.onStartStop(onStartStop);
  msc.mediaPresent(true);
  msc.begin(card->getSectorCount(), card->getSectorSize());
}

void stopMSC()
{
  msc.end();
}
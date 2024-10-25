#include <Arduino.h>
#include "USB.h"
#include "USBMSC.h"

class SDCard;

extern USBMSC msc;
extern USBCDC Serial;

void setupUSB(SDCard *_card);
void startMSC();
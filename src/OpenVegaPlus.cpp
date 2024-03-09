/*=====================================================================
 * Open Vega+ Emulator. Handheld ESP32 based hardware with
 * ZX Spectrum emulation
 *
 * (C) 2019 Alvaro Alea Fernandez
 *
 * This program is free software; redistributable under the terms of
 * the GNU General Public License Version 2
 *
 * Based on the Aspectrum emulator Copyright (c) 2000 Santiago Romero Iglesias
 * Use Adafruit's IL9341 and GFX libraries.
 * Compile as ESP32 Wrover Module
 *======================================================================
 */
#include <WiFi.h>
#include "SPIFFS.h"
#include "SPI.h"
#include "gui.h"

#define USE_ESPI

#include <TFT_eSPI.h>
TFT_eSPI tft = TFT_eSPI();


#include "hardware.h"
#include "snaps.h"
#include "spectrum.h"
#include "z80.h"

/* Definimos unos objetos
 *  spectrumZ80 es el procesador en si
 *  mem es la memoria ROM/RAM del ordenador
 *  hwopt es la configuracion de la maquina a emular esto se guarda en los snapshots
 *  emuopt es la configuracion del emulador en si, no se guarda con el snapshot
 */
Z80Regs spectrumZ80;
tipo_mem mem;
tipo_hwopt hwopt;
tipo_emuopt emuopt;
uint16_t *frameBuffer = NULL;

void ula_tick();


const int screenWidth = TFT_HEIGHT;
const int screenHeight = TFT_WIDTH;
const int borderWidth = (screenWidth-256)/2;
const int borderHeight = (screenHeight-192)/2;

const int specpal565[16] = {
    0x0000, 0x1B00, 0x00B8, 0x17B8, 0xE005, 0xF705, 0xE0BD, 0x18C6, 0x0000, 0x1F00, 0x00F8, 0x1FF8, 0xE007, 0xFF07, 0xE0FF, 0xFFFF};


void drawDisplay(void *pvParameters)
{
  uint32_t frames = 0;
  int32_t frame_millis = 0;
  uint16_t lastBorderColor = 0;
  tft.startWrite();
  tft.fillScreen(TFT_BLACK);
  tft.endWrite();
  while (1)
  {
    if(frame_millis == 0) {
      frame_millis = millis();
    }
    tft.startWrite();
    tft.dmaWait();
    // do the border
    uint8_t borderColor = hwopt.BorderColor &B00000111;
    // TODO - can the border color be bright?
    uint16_t tftColor = specpal565[borderColor];
    if (tftColor != lastBorderColor) {
      // do the border with some simple rects - no need to push pixels for a solid color
      tft.fillRect(0, 0, screenWidth, borderHeight, tftColor);
      tft.fillRect(0, screenHeight-borderHeight, screenWidth, borderHeight, tftColor);
      tft.fillRect(0, borderHeight, borderWidth, screenHeight-borderHeight, tftColor);
      tft.fillRect(screenWidth-borderWidth, borderHeight, borderWidth, screenHeight-borderHeight, tftColor);
      lastBorderColor = tftColor;
    }
    // do the pixels
    uint8_t *attrBase = mem.p + mem.vo[hwopt.videopage] + 0x1800;
    uint8_t *pixelBase = mem.p + mem.vo[hwopt.videopage];
    for(int attrY = 0; attrY < 192/8; attrY++) {
      bool dirty = false;
      for(int attrX = 0; attrX < 256/8; attrX++) {
        // read the value of the attribute
        uint8_t attr = *(attrBase + 32 * attrY + attrX) ;
        uint8_t inkColor = attr & B00000111;
        uint8_t paperColor = (attr & B00111000) >> 3;
        if ((attr & B01000000) != 0)
        {
          inkColor = inkColor + 8;
          paperColor = paperColor + 8;
        }
        uint16_t tftInkColor = specpal565[inkColor];
        uint16_t tftPaperColor = specpal565[paperColor];
        for(int y = attrY*8; y<attrY*8+8; y++) {
          // read the value of the pixels
          int col = (attrX*8 & B11111000) >> 3;
          int scan = (y & B11000000) + ((y & B111) << 3) + ((y & B111000) >> 3);
          uint8_t row = *(pixelBase + 32 * scan + col);
          for(int x = attrX*8; x<attrX*8+8; x++) {
            uint16_t *pixelAddress = frameBuffer + x + 256 * y;
            if (row & (1 << (7 - (x & 7)))) {
              if (tftInkColor != *pixelAddress) {
                *pixelAddress = tftInkColor;
                dirty = true;
              }
            } else {
              if (tftPaperColor != *pixelAddress) {
                *pixelAddress = tftPaperColor;
                dirty = true;
              }
            }
          }
        }
      }
      if (dirty) {
        // push out this block of pixels 256 * 8
        tft.dmaWait();
        tft.setWindow(borderWidth, borderHeight + attrY*8, borderWidth+255, borderHeight + attrY*8+7);
        tft.pushPixelsDMA(frameBuffer + attrY * 8 * 256, 256 * 8);
      }
    }
    tft.endWrite();
    frames++;
    if (millis() - frame_millis > 1000) {
      vTaskDelay(1);
      Serial.printf("Frame rate=%d\n", frames);
      frames = 0;
      frame_millis = millis();
    }
  }
}

void z80Runner(void *pvParameter) {
  long pend_ula_ticks = 0;
  const uint8_t SoundTable[4] = {0, 2, 0, 2};
  uint32_t c = 0;
  int lastMillis = 0;
  while(1) {
    if (lastMillis == 0) {
      lastMillis = millis();
    }
    c += Z80Run(&spectrumZ80, 224);
    if (millis() - lastMillis > 1000) {
      lastMillis = millis();
      Serial.printf("Executed at %.2fMHz cycles\n", c/1000000.0);
      c = 0;
    }
    vTaskDelay(1);
  }
}

void setup(void)
{
  Serial.begin(115200);

  // turn on the TFT
  pinMode(TFT_POWER, OUTPUT);
  digitalWrite(TFT_POWER, LOW);

  for (int i = 0; i < 5; i++)
  {
    delay(1000);
    AS_printf("Waiting %i\n", i);
  }
  AS_printf("OpenVega+ Boot!\n");
  keypad_i2c_init();
  // Initialize SPIFFS
  if (!SPIFFS.begin(true))
  {
    AS_printf("An Error has occurred while mounting SPIFFS\n");
    return;
  }

  Serial.println("Border Width: " + String(borderWidth));
  Serial.println("Border Height: " + String(borderHeight));

  // This is supous that provide por CPU to the program.
  // WiFi.mode(WIFI_OFF);
  // btStop();

  tft.begin();
  // DMA - should work with ESP32, STM32F2xx/F4xx/F7xx processors  >>>>>> DMA IS FOR SPI DISPLAYS ONLY <<<<<<
  tft.initDMA(); // Initialise the DMA engine
  tft.setRotation(3);
  tft.fillScreen(TFT_WHITE);
  show_splash(tft);
  AS_printf("Total heap: ");
  AS_print(ESP.getHeapSize());
  AS_printf("\n");
  AS_printf("Free heap: ");
  AS_print(ESP.getFreeHeap());
  AS_printf("\n");
  AS_printf("Total PSRAM: ");
  AS_print(ESP.getPsramSize());
  AS_printf("\n");
  AS_printf("Free PSRAM: ");
  AS_print(ESP.getFreePsram());
  AS_printf("\n");

  frameBuffer = (uint16_t *)malloc(256 * 192 * sizeof(uint16_t));
  if (frameBuffer == NULL)
  {
    AS_printf("Error! memory not allocated for screenbuffer.\n");
    delay(10000);
  }
  memset(frameBuffer, 0, 256 * 192 * sizeof(uint16_t));

  AS_printf("\nDespues\n");
  AS_printf("Total heap: ");
  AS_print(ESP.getHeapSize());
  AS_printf("\n");
  AS_printf("Free heap: ");
  AS_print(ESP.getFreeHeap());
  AS_printf("\n");
  AS_printf("Total PSRAM: ");
  AS_print(ESP.getPsramSize());
  AS_printf("\n");
  AS_printf("Free PSRAM: ");
  AS_print(ESP.getFreePsram());
  AS_printf("\n");

  // FIXME porque 69888?? ¿quiza un frame, para que pinte la pantalla una vez?
  Z80Reset(&spectrumZ80, 69888);
  Z80FlagTables();
  AS_printf("Z80 Initialization completed\n");

  init_spectrum(SPECMDL_48K, "/48.rom");
  reset_spectrum(&spectrumZ80);
  Load_SNA(&spectrumZ80, "/manic.sna");
  //  Load_SNA(&spectrumZ80,"/t1.sna");

  delay(2000);
  AS_printf("Entrando en el loop\n");

  xTaskCreatePinnedToCore(drawDisplay, "drawDisplay", 16384, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(z80Runner, "z80Runner", 16384, NULL, 5, NULL, 1);
}

void loop(void)
{
  vTaskDelay(10000);
}

void ula_tick()
{
  uint8_t color;
  uint8_t pixel;
  int col, fil, scan, inkOpap;
  static int px = 31;  // emulator hardware pixel to be paint, x coordinate
  static int py = 24;  // emulator hardware pixel to be paint, y coordinate
  static int ch = 447; // 0-447 is the horizontal bean position
  static int cv = 312; // 0-312 is the vertical bean position
  static int cf = 0;   // 0-32 counter for flash
                       //  static int cf2=0;
                       //  static int cf3=0;
  static uint8_t attr; // last attrib memory read

  ch++;
  if (ch >= 448)
    ch = 0;
  if (ch == 416)
    px = 0;
  if ((ch <= 288) or (ch > 416))
    px++;

  if (ch == 0)
  {
    cv++;
    if (cv >= 313)
    {
      cv = 0;
      leebotones(tft, emuopt, spectrumZ80);
    }
    if (cv == 288)
    {
      py = 0;
      cf++;
      if (cf >= 32)
        cf = 0;
    }
    if ((cv < 218) or (cv > 288))
      py++;
  }
  if ((ch == 0) and (cv == 248))
    spectrumZ80.petint = 1;

  if ((cv >= 192) or (ch >= 256))
  {
    hwopt.portFF = 0xFF;
    color = hwopt.BorderColor;
  }
  else
  {
    pixel = 7 - (ch & B111);
    col = (ch & B11111000) >> 3;
    fil = cv >> 3;
    scan = (cv & B11000000) + ((cv & B111) << 3) + ((cv & B111000) >> 3);
    // FIXME: Two acces to mem in same tick, really ATTR is Access a pixel early
    //        so a reading must be done when cv<192 and ch=447 for 0x5800 + 32*fil
    //        later attr for col+1 on ch & B111 == 0b111
    //        deberia ir en un if separado para no leer 33 valores, el ultimo no hace falta asi que ch<250
    if ((ch & B111) == 0b000)
    {
      attr = readvmem(0x1800 + 32 * fil + col, hwopt.videopage);
    }
    if ((ch & B111) == 0b000)
    {
      hwopt.portFF = readvmem(0x0000 + 32 * scan + col, hwopt.videopage);
    }
    inkOpap = (hwopt.portFF >> pixel) & 1;
    // FLASH
    if ((cf >= 16) && ((attr & B10000000) != 0))
      inkOpap = !inkOpap;
    // INK or PAPER
    if (inkOpap != 0)
    {
      color = attr & B000111;
    }
    else
    {
      color = (attr & B111000) >> 3;
    }
    // BRIGHT
    if ((attr & B01000000) != 0)
    {
      color = color + 8;
    }
  }
  if ((px < 256) and (py < 192))
  {
    uint16_t tftColor = specpal565[color];
    uint16_t *pixelAddress = frameBuffer + px + 256 * py;
    if (tftColor != *pixelAddress) {
      *pixelAddress = tftColor;
    }
  }
}

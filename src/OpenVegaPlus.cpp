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
bool dirty[240] = {false};

void ula_tick(TFT_eSPI &tft);

void setup(void)
{
  Serial.begin(115200);

  // turn on the TFT
  pinMode(TFT_POWER, OUTPUT);
  digitalWrite(TFT_POWER, LOW);

  for (int i = 0; i < 10; i++)
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

  // This is supous that provide por CPU to the program.
  // WiFi.mode(WIFI_OFF);
  // btStop();

  tft.begin();
  // DMA - should work with ESP32, STM32F2xx/F4xx/F7xx processors  >>>>>> DMA IS FOR SPI DISPLAYS ONLY <<<<<<
  tft.initDMA(); // Initialise the DMA engine
  tft.setRotation(3);
  tft.fillScreen(TFT_WHITE);
  tft.setSwapBytes(true);
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

  frameBuffer = (uint16_t *)malloc(280 * 240 * sizeof(uint16_t));
  if (frameBuffer == NULL)
  {
    AS_printf("Error! memory not allocated for screenbuffer.\n");
    delay(10000);
  }
  memset(frameBuffer, 0, 280 * 240 * sizeof(uint16_t));

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
}

long pend_ula_ticks = 0;
const uint8_t SoundTable[4] = {0, 2, 0, 2};

// void draw_scanline(){
void loop(void)
{
  uint16_t c = 0;
  // dividimos el scanline en la parte central, y los bordes y retrazo, para la emulacion del puerto FF

  pend_ula_ticks += 256;
  c = Z80Run(&spectrumZ80, 128);
  while (pend_ula_ticks > 0)
  {
    pend_ula_ticks--;
    ula_tick(tft);
  }

  pend_ula_ticks += 192;
  c = Z80Run(&spectrumZ80, 96);
  while (pend_ula_ticks > 0)
  {
    pend_ula_ticks--;
    ula_tick(tft);
  }

  //  AS_printf("PC=%i\n",c);
  // Sound on each scanline means 15.6Khz, not bad...
  if (emuopt.sonido == 1)
  {
    // TODO - add sound
    // dac_output_voltage(DAC_CHANNEL_1, SoundTable[hwopt.SoundBits]);
  }
}

void ula_tick(TFT_eSPI &tft)
{

  const int specpal565[16] = {
      0x0000, 0x001B, 0xB800, 0xB817, 0x05E0, 0x05F7, 0xBDE0, 0xC618, 0x0000, 0x001F, 0xF800, 0xF81F, 0x07E0, 0x07FF, 0xFFE0, 0xFFFF};

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
  // Paint the real screen
  if ((px == 0) and (py == 0))
    tft.startWrite();

  if ((px < 294) and (py < 240))
  {
    uint16_t tftColor = specpal565[color];
    uint16_t *pixelAddress = frameBuffer + px - 14 + 280 * py;
    if (tftColor != *pixelAddress) {
      *pixelAddress = tftColor;
      dirty[py] = true;
    }
  }

  if ((px == 294) and (py == 240))
  {
    // tft.startWrite();
    for(int y = 0; y<240; y++) {
      if (dirty[y]) {
        tft.setWindow(0, y, 279, y);
        tft.pushPixelsDMA(frameBuffer + 280*y, 280);
        dirty[y] = false;
      }
    }
    // tft.setWindow(0, 0, 279, 239);
    // tft.pushPixelsDMA(frameBuffer, 280*240);
    // tft.endWrite();
  }
}

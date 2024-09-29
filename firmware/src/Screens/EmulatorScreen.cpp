#include <Arduino.h>
#include "../TFT/TFTDisplay.h"
#include "../Emulator/spectrum.h"
#include "../Emulator/snaps.h"
#include "../AudioOutput/AudioOutput.h"
#include "EmulatorScreen.h"

const int screenWidth = TFT_HEIGHT;
const int screenHeight = TFT_WIDTH;
#ifdef SCALE_SCREEN
// scale the screen by 1.5 times
const int borderWidth = (screenWidth - 384) / 2;
const int borderHeight = (screenHeight - 288) / 2;
#else
const int borderWidth = (screenWidth - 256) / 2;
const int borderHeight = (screenHeight - 192) / 2;
#endif

const uint16_t specpal565[16] = {
    0x0000, 0x1B00, 0x00B8, 0x17B8, 0xE005, 0xF705, 0xE0BD, 0x18C6, 0x0000, 0x1F00, 0x00F8, 0x1FF8, 0xE007, 0xFF07, 0xE0FF, 0xFFFF};

uint16_t flashTimer = 0;
uint16_t lastBorderColor = -1;
#ifdef SCALE_SCREEN
uint16_t scaledBuffer[384 * 12];
#endif

void drawScreen(EmulatorScreen *emulatorScreen)
{
  TFTDisplay &tft = emulatorScreen->m_tft;
  ZXSpectrum *machine = emulatorScreen->machine;

  tft.startWrite();
#ifdef USE_DMA
  tft.dmaWait();
#endif
  // do the border
  uint8_t borderColor = machine->hwopt.BorderColor & B00000111;
  uint16_t tftColor = specpal565[borderColor];
  // swap the byte order
  tftColor = (tftColor >> 8) | (tftColor << 8);
  if (tftColor != lastBorderColor)
  {
    // do the border with some simple rects - no need to push pixels for a solid color
    tft.fillRect(0, 0, screenWidth, borderHeight, tftColor);
    tft.fillRect(0, screenHeight - borderHeight, screenWidth, borderHeight, tftColor);
    tft.fillRect(0, borderHeight, borderWidth, screenHeight - borderHeight, tftColor);
    tft.fillRect(screenWidth - borderWidth, borderHeight, borderWidth, screenHeight - borderHeight, tftColor);
    lastBorderColor = tftColor;
  }
  // do the pixels
  uint8_t *attrBase = machine->mem.currentScreen + 0x1800;
  uint8_t *pixelBase = machine->mem.currentScreen;
  uint8_t *attrBaseCopy = emulatorScreen->screenBuffer + 0x1800;
  uint8_t *pixelBaseCopy = emulatorScreen->screenBuffer;
  for (int attrY = 0; attrY < 192 / 8; attrY++)
  {
    bool dirty = false;
    for (int attrX = 0; attrX < 256 / 8; attrX++)
    {
      // read the value of the attribute
      uint8_t attr = *(attrBase + 32 * attrY + attrX);
      uint8_t inkColor = attr & B00000111;
      uint8_t paperColor = (attr & B00111000) >> 3;
      // check for changes in the attribute
      if (attr != *(attrBaseCopy + 32 * attrY + attrX))
      {
        dirty = true;
        *(attrBaseCopy + 32 * attrY + attrX) = attr;
      }
      if ((attr & B01000000) != 0)
      {
        inkColor = inkColor + 8;
        paperColor = paperColor + 8;
      }
      // if we are flashing and the flash timer is less than 16 then swap ink and paper
      if ((attr & B10000000) != 0 && flashTimer < 16)
      {
        uint8_t temp = inkColor;
        inkColor = paperColor;
        paperColor = temp;
        // flash indicates that we are changing
        dirty = true;
      }
      uint16_t tftInkColor = specpal565[inkColor];
      uint16_t tftPaperColor = specpal565[paperColor];
      for (int y = 0; y < 8; y++)
      {
        // read the value of the pixels
        int screenY = attrY * 8 + y;
        int col = ((attrX * 8) & B11111000) >> 3;
        int scan = (screenY & B11000000) + ((screenY & B111) << 3) + ((screenY & B111000) >> 3);
        uint8_t row = *(pixelBase + 32 * scan + col);
        uint8_t rowCopy = *(pixelBaseCopy + 32 * scan + col);
        // check for changes in the pixel data
        if (row != rowCopy)
        {
          dirty = true;
          *(pixelBaseCopy + 32 * scan + col) = row;
        }
        uint16_t *pixelAddress = emulatorScreen->dmaBuffer1 + 256 * y + attrX * 8;
        for (int x = 0; x <8; x++)
        {
          if (row & 128)
          {
            *pixelAddress = tftInkColor;
          }
          else
          {
            *pixelAddress = tftPaperColor;
          }
          pixelAddress++;
          row = row << 1;
        }
      }
    }
    if (dirty || emulatorScreen->firstDraw)
    {
// push out this block of pixels 256 * 8
#ifdef USE_DMA
      tft.dmaWait();
      tft.setWindow(borderWidth, borderHeight + attrY * 8, borderWidth + 255, borderHeight + attrY * 8 + 7);
      tft.pushPixelsDMA(emulatorScreen->dmaBuffer1, 256 * 8);
#else
#ifdef SCALE_SCREEN
      // scale the buffer by 1.5 times (repeat every other pixel horizontally and vertically)
      int dstY = 0;
      for (int y = 0; y < 8; y++)
      {
        int dstX = 0;
        for (int x = 0; x < 256; x++)
        {
          scaledBuffer[dstY * 384 + dstX] = emulatorScreen->dmaBuffer1[y * 256 + x];
          if (x % 2 == 0)
          {
            dstX++;
            scaledBuffer[dstY * 384 + dstX] = emulatorScreen->dmaBuffer1[y * 256 + x];
          }
          dstX++;
        }
        if (y % 2 == 0) {
          dstY++;
          for(int x = 0; x < 384; x++) {
            scaledBuffer[dstY * 384 + x] = scaledBuffer[(dstY - 1) * 384 + x];
          }
        }
        dstY++;
      }
      tft.setWindow(borderWidth, borderHeight + attrY * 12, borderWidth + 383, borderHeight + attrY * 12 + 11);
      tft.pushPixels(scaledBuffer, 384 * 12);
#else
      tft.setWindow(borderWidth, borderHeight + attrY * 8, borderWidth + 255, borderHeight + attrY * 8 + 7);
      tft.pushPixels(emulatorScreen->dmaBuffer1, 256 * 8);
#endif
#endif
      // swap the DMA buffers
      uint16_t *temp = emulatorScreen->dmaBuffer1;
      emulatorScreen->dmaBuffer1 = emulatorScreen->dmaBuffer2;
      emulatorScreen->dmaBuffer2 = temp;
    }
  }
  #ifdef USE_DMA
    tft.dmaWait();
  #endif
  tft.endWrite();
  emulatorScreen->firstDraw = false;
}

void drawDisplay(void *pvParameters)
{
  EmulatorScreen *emulatorScreen = (EmulatorScreen *)pvParameters;
  while (1)
  {
    if (!emulatorScreen->isRunning)
    {
      vTaskDelay(100 / portTICK_PERIOD_MS);
      continue;
    }
    if(xSemaphoreTake(emulatorScreen->m_displaySemaphore, portMAX_DELAY)) {
      drawScreen(emulatorScreen);
      emulatorScreen->frameCount++;
      flashTimer++;
      if (flashTimer == 32)
      {
        flashTimer = 0;
      }
    }
  }
}

void z80Runner(void *pvParameter)
{
  EmulatorScreen *emulatorScreen = (EmulatorScreen *)pvParameter;
  while (1)
  {
    if (emulatorScreen->isRunning)
    {
      emulatorScreen->cycleCount += emulatorScreen->machine->runForFrame(emulatorScreen->m_audioOutput, emulatorScreen->audioFile);
      // draw a frame
      xSemaphoreGive(emulatorScreen->m_displaySemaphore);
      // uint32_t evt = 0;
      // xQueueSend(emulatorScreen->frameRenderTimerQueue, &evt, 0);
    }
    else
    {
      vTaskDelay(100 / portTICK_PERIOD_MS);
    }
  }
}

EmulatorScreen::EmulatorScreen(TFTDisplay &tft, AudioOutput *audioOutput) : Screen(tft, audioOutput)
{
  dmaBuffer1 = (uint16_t *)heap_caps_malloc(screenWidth * 8 * sizeof(uint16_t), MALLOC_CAP_DMA);
  dmaBuffer2 = (uint16_t *)heap_caps_malloc(screenWidth * 8 * sizeof(uint16_t), MALLOC_CAP_DMA);
  screenBuffer = (uint8_t *)malloc(6912);
  if (screenBuffer == NULL)
  {
    Serial.println("Failed to allocate screen buffer");
  }
  memset(screenBuffer, 0, 6912);
  m_displaySemaphore = xSemaphoreCreateBinary();
}

void EmulatorScreen::run(std::string snaPath)
{
  // audioFile = fopen("/fs/audio.raw", "wb");
  memset(dmaBuffer1, 0, screenWidth * 8 * sizeof(uint16_t));
  memset(dmaBuffer2, 0, screenWidth * 8 * sizeof(uint16_t));
  memset(screenBuffer, 0, 6192);
  machine = new ZXSpectrum();
  machine->reset();
  machine->init_spectrum(SPECMDL_48K);
  machine->reset_spectrum(machine->z80Regs);
  Load(machine, snaPath.c_str());
  m_tft.fillScreen(TFT_WHITE);
  firstDraw = true;
  isRunning = true;
  // tasks to do the work
  xTaskCreatePinnedToCore(drawDisplay, "drawDisplay", 8192, this, 1, NULL, 1);
  xTaskCreatePinnedToCore(z80Runner, "z80Runner", 8192, this, 5, NULL, 0);
}

void EmulatorScreen::run48K() {
  memset(dmaBuffer1, 0, screenWidth * 8 * sizeof(uint16_t));
  memset(dmaBuffer2, 0, screenWidth * 8 * sizeof(uint16_t));
  memset(screenBuffer, 0, 6192);
  machine = new ZXSpectrum();
  machine->reset();
  machine->init_spectrum(SPECMDL_48K);
  machine->reset_spectrum(machine->z80Regs);
  m_tft.fillScreen(TFT_WHITE);
  firstDraw = true;
  isRunning = true;
  // tasks to do the work
  xTaskCreatePinnedToCore(drawDisplay, "drawDisplay", 8192, this, 1, NULL, 1);
  xTaskCreatePinnedToCore(z80Runner, "z80Runner", 8192, this, 5, NULL, 0);
}

void EmulatorScreen::run128K() {
  memset(dmaBuffer1, 0, screenWidth * 8 * sizeof(uint16_t));
  memset(dmaBuffer2, 0, screenWidth * 8 * sizeof(uint16_t));
  memset(screenBuffer, 0, 6192);
  machine = new ZXSpectrum();
  machine->reset();
  machine->init_spectrum(SPECMDL_128K);
  machine->reset_spectrum(machine->z80Regs);
  m_tft.fillScreen(TFT_WHITE);
  firstDraw = true;
  isRunning = true;
  // tasks to do the work
  xTaskCreatePinnedToCore(drawDisplay, "drawDisplay", 8192, this, 1, NULL, 1);
  xTaskCreatePinnedToCore(z80Runner, "z80Runner", 8192, this, 5, NULL, 0);
}

void EmulatorScreen::stop()
{
  isRunning = false;
}

void EmulatorScreen::updatekey(SpecKeys key, uint8_t state)
{
  // if (audioFile) {
  //   isRunning = false;
  //   vTaskDelay(1000 / portTICK_PERIOD_MS);
  //   fclose(audioFile);
  //   audioFile = NULL;
  //   Serial.printf("Audio file closed\n");
  // }
  if (isRunning) {
    machine->updatekey(key, state);
  }
}
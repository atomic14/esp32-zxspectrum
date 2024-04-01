#include <Arduino.h>
#include <TFT_eSPI.h>
#include "../Emulator/spectrum.h"
#include "../Emulator/snaps.h"
#include "../AudioOutput/AudioOutput.h"
#include "EmulatorScreen.h"

const int screenWidth = TFT_HEIGHT;
const int screenHeight = TFT_WIDTH;
const int borderWidth = (screenWidth - 256) / 2;
const int borderHeight = (screenHeight - 192) / 2;

const uint16_t specpal565[16] = {
    0x0000, 0x1B00, 0x00B8, 0x17B8, 0xE005, 0xF705, 0xE0BD, 0x18C6, 0x0000, 0x1F00, 0x00F8, 0x1FF8, 0xE007, 0xFF07, 0xE0FF, 0xFFFF};

uint16_t flashTimer = 0;
uint16_t lastBorderColor = 0;

void drawScreen(EmulatorScreen *emulatorScreen)
{
  TFT_eSPI &tft = emulatorScreen->m_tft;
  ZXSpectrum *machine = emulatorScreen->machine;
  
  tft.startWrite();
#ifdef USE_DMA
  tft.dmaWait();
#endif
  // do the border
  uint8_t borderColor = machine->hwopt.BorderColor & B00000111;
  uint16_t tftColor = specpal565[borderColor];
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
  uint8_t *attrBase = machine->mem.p + machine->mem.vo[machine->hwopt.videopage] + 0x1800;
  uint8_t *pixelBase = machine->mem.p + machine->mem.vo[machine->hwopt.videopage];
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
    if (dirty)
    {
// push out this block of pixels 256 * 8
#ifdef USE_DMA
      tft.dmaWait();
      tft.setWindow(borderWidth, borderHeight + attrY * 8, borderWidth + 255, borderHeight + attrY * 8 + 7);
      tft.pushPixelsDMA(emulatorScreen->dmaBuffer1, 256 * 8);
#else
      tft.setWindow(borderWidth, borderHeight + attrY * 8, borderWidth + 255, borderHeight + attrY * 8 + 7);
      tft.pushPixels(emulatorScreen->dmaBuffer1, 256 * 8);
#endif
      // swap the DMA buffers
      uint16_t *temp = emulatorScreen->dmaBuffer1;
      emulatorScreen->dmaBuffer1 = emulatorScreen->dmaBuffer2;
      emulatorScreen->dmaBuffer2 = temp;
    }
  }
  tft.endWrite();
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

EmulatorScreen::EmulatorScreen(TFT_eSPI &tft, AudioOutput *audioOutput) : Screen(tft, audioOutput)
{
  machine = new ZXSpectrum();
  machine->reset();
  Serial.printf("Z80 Initialization completed\n");
  machine->init_spectrum(SPECMDL_48K, "/fs/48.rom");
  machine->reset_spectrum(machine->z80Regs);

  // frameBuffer = (uint16_t *)malloc(256 * 192 * sizeof(uint16_t));
  // if (frameBuffer == NULL)
  // {
  //   Serial.printf("Error! memory not allocated for screenbuffer.\n");
  // }
  // memset(frameBuffer, 0, 256 * 192 * sizeof(uint16_t));
  dmaBuffer1 = (uint16_t *)heap_caps_malloc(256 * 8 * sizeof(uint16_t), MALLOC_CAP_DMA);
  dmaBuffer2 = (uint16_t *)heap_caps_malloc(256 * 8 * sizeof(uint16_t), MALLOC_CAP_DMA);
  screenBuffer = (uint8_t *)malloc(6192);
  memset(screenBuffer, 0, 6192);

  Serial.printf("Entrando en el loop\n");
  // cpuRunTimerQueue = xQueueCreate(10, sizeof(uint32_t));
  // frameRenderTimerQueue = xQueueCreate(10, sizeof(uint32_t));
  m_displaySemaphore = xSemaphoreCreateBinary();
  // tasks to do the work
  xTaskCreatePinnedToCore(drawDisplay, "drawDisplay", 16384, this, 1, NULL, 1);
  // use the I2S output to control the frame rate
  xTaskCreatePinnedToCore(z80Runner, "z80Runner", 16384, this, 5, NULL, 0);
}

void EmulatorScreen::run(std::string snaPath)
{
  isRunning = false;
  machine->reset();
  machine->init_spectrum(SPECMDL_48K, "/fs/48.rom");
  machine->reset_spectrum(machine->z80Regs);
  Load_SNA(machine, snaPath.c_str());
  memset(dmaBuffer1, 0, 256 * 8 * sizeof(uint16_t));
  memset(dmaBuffer2, 0, 256 * 8 * sizeof(uint16_t));
  memset(screenBuffer, 0, 6192);
  m_tft.fillScreen(TFT_BLACK);
  isRunning = true;
  audioFile = fopen("/fs/audio.raw", "wb");
  if (!audioFile)
  {
    Serial.printf("Error opening audio file\n");
  }
}

void EmulatorScreen::stop()
{
  isRunning = false;
}

void EmulatorScreen::updatekey(uint8_t key, uint8_t state)
{
  // if (audioFile) {
  //   isRunning = false;
  //   vTaskDelay(1000 / portTICK_PERIOD_MS);
  //   fclose(audioFile);
  //   audioFile = NULL;
  //   Serial.printf("Audio file closed\n");
  // }
  machine->updatekey(key, state);
}
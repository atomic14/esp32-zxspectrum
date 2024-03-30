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

const int specpal565[16] = {
    0x0000, 0x1B00, 0x00B8, 0x17B8, 0xE005, 0xF705, 0xE0BD, 0x18C6, 0x0000, 0x1F00, 0x00F8, 0x1FF8, 0xE007, 0xFF07, 0xE0FF, 0xFFFF};

void drawDisplay(void *pvParameters)
{
  EmulatorScreen *emulatorScreen = (EmulatorScreen *)pvParameters;
  uint16_t flashTimer = 0;
  uint16_t lastBorderColor = 0;
  TFT_eSPI &tft = emulatorScreen->m_tft;
  // tft.startWrite();
  // tft.fillScreen(TFT_BLACK);
  // tft.endWrite();
  ZXSpectrum *machine = emulatorScreen->machine;
  uint16_t *frameBuffer = emulatorScreen->frameBuffer;
  while (1)
  {
    if (!emulatorScreen->isRunning)
    {
      vTaskDelay(100 / portTICK_PERIOD_MS);
      continue;
    }
    uint32_t evt;
    if (xQueueReceive(emulatorScreen->frameRenderTimerQueue, &evt, portMAX_DELAY))
    {
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
      for (int attrY = 0; attrY < 192 / 8; attrY++)
      {
        bool dirty = false;
        for (int attrX = 0; attrX < 256 / 8; attrX++)
        {
          // read the value of the attribute
          uint8_t attr = *(attrBase + 32 * attrY + attrX);
          uint8_t inkColor = attr & B00000111;
          uint8_t paperColor = (attr & B00111000) >> 3;
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
          }
          uint16_t tftInkColor = specpal565[inkColor];
          uint16_t tftPaperColor = specpal565[paperColor];
          for (int y = attrY * 8; y < attrY * 8 + 8; y++)
          {
            // read the value of the pixels
            int col = (attrX * 8 & B11111000) >> 3;
            int scan = (y & B11000000) + ((y & B111) << 3) + ((y & B111000) >> 3);
            uint8_t row = *(pixelBase + 32 * scan + col);
            for (int x = attrX * 8; x < attrX * 8 + 8; x++)
            {
              uint16_t *pixelAddress = frameBuffer + x + 256 * y;
              if (row & (1 << (7 - (x & 7))))
              {
                if (tftInkColor != *pixelAddress)
                {
                  *pixelAddress = tftInkColor;
                  dirty = true;
                }
              }
              else
              {
                if (tftPaperColor != *pixelAddress)
                {
                  *pixelAddress = tftPaperColor;
                  dirty = true;
                }
              }
            }
          }
        }
        if (dirty)
        {
// push out this block of pixels 256 * 8
#ifdef USE_DMA
          tft.dmaWait();
          tft.setWindow(borderWidth, borderHeight + attrY * 8, borderWidth + 255, borderHeight + attrY * 8 + 7);
          tft.pushPixelsDMA(frameBuffer + attrY * 8 * 256, 256 * 8);
#else
          tft.setWindow(borderWidth, borderHeight + attrY * 8, borderWidth + 255, borderHeight + attrY * 8 + 7);
          tft.pushPixels(frameBuffer + attrY * 8 * 256, 256 * 8);
#endif
        }
      }
      tft.endWrite();
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
    if (emulatorScreen->isRunning) {
      emulatorScreen->cycleCount += emulatorScreen->machine->runForFrame(emulatorScreen->m_audioOutput);
      // draw a frame
      uint32_t evt = 0;

      xQueueSend(emulatorScreen->frameRenderTimerQueue, &evt, portMAX_DELAY);
    } else {
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

  frameBuffer = (uint16_t *)malloc(256 * 192 * sizeof(uint16_t));
  if (frameBuffer == NULL)
  {
    Serial.printf("Error! memory not allocated for screenbuffer.\n");
  }
  memset(frameBuffer, 0, 256 * 192 * sizeof(uint16_t));

  Serial.printf("Entrando en el loop\n");
  // cpuRunTimerQueue = xQueueCreate(10, sizeof(uint32_t));
  frameRenderTimerQueue = xQueueCreate(10, sizeof(uint32_t));
  // tasks to do the work
  xTaskCreatePinnedToCore(drawDisplay, "drawDisplay", 16384, this, 1, NULL, 1);
  // use the I2S output to control the frame rate
  xTaskCreatePinnedToCore(z80Runner, "z80Runner", 16384, this, 5, NULL, 0);
}

void EmulatorScreen::run(std::string snaPath)
{
  machine->reset();
  machine->init_spectrum(SPECMDL_48K, "/fs/48.rom");
  machine->reset_spectrum(machine->z80Regs);
  Load_SNA(machine, snaPath.c_str());
  memset(frameBuffer, 0, 256 * 192 * sizeof(uint16_t));
  m_tft.fillScreen(TFT_BLACK);
  isRunning = true;
}

void EmulatorScreen::stop()
{
  isRunning = false;
}


void EmulatorScreen::updatekey(uint8_t key, uint8_t state) {
  machine->updatekey(key, state);
}
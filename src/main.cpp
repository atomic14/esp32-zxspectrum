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
#include <TFT_eSPI.h>
#include <esp_err.h>
#include "SPIFFS.h"
#include "SPI.h"
#include "driver/timer.h"
#include "AudioOutput/I2SOutput.h"
#include "AudioOutput/PDMOutput.h"
#include "AudioOutput/DACOutput.h"
#include "AudioOutput/BuzzerOutput.h"
#include "z80/snaps.h"
#include "spectrum.h"
#include "Nunchuk/wii_i2c.h"

TFT_eSPI tft = TFT_eSPI();
AudioOutput *audioOutput = NULL;
ZXSpectrum *machine = NULL;

unsigned int controller_type = 0;

uint16_t *frameBuffer = NULL;

xQueueHandle cpuRunTimerQueue;
xQueueHandle frameRenderTimerQueue;

const int screenWidth = TFT_HEIGHT;
const int screenHeight = TFT_WIDTH;
const int borderWidth = (screenWidth - 256) / 2;
const int borderHeight = (screenHeight - 192) / 2;

const int specpal565[16] = {
    0x0000, 0x1B00, 0x00B8, 0x17B8, 0xE005, 0xF705, 0xE0BD, 0x18C6, 0x0000, 0x1F00, 0x00F8, 0x1FF8, 0xE007, 0xFF07, 0xE0FF, 0xFFFF};

uint32_t c = 0;

void drawDisplay(void *pvParameters)
{
  uint32_t frames = 0;
  uint16_t flashTimer = 0;
  int32_t frame_millis = 0;
  uint16_t lastBorderColor = 0;
  tft.startWrite();
  tft.fillScreen(TFT_BLACK);
  tft.endWrite();
  while (1)
  {
    uint32_t evt;
    if (xQueueReceive(frameRenderTimerQueue, &evt, portMAX_DELAY))
    {
      if (frame_millis == 0)
      {
        frame_millis = millis();
      }
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
      frames++;
      flashTimer++;
      if (flashTimer == 32)
      {
        flashTimer = 0;
      }
      const unsigned char *data = wii_i2c_read_state();
      wii_i2c_request_state();
      if (data)
      {
        switch (controller_type)
        {
        case WII_I2C_IDENT_NUNCHUK:
        {
          wii_i2c_nunchuk_state state;
          wii_i2c_decode_nunchuk(data, &state);
          if (state.x > 50)
          { // going right
            machine->updatekey(JOYK_RIGHT, 1);
          }
          else if (state.x < -50)
          { // going left
            machine->updatekey(JOYK_LEFT, 1);
          }
          else
          {
            machine->updatekey(JOYK_LEFT, 0);
            machine->updatekey(JOYK_RIGHT, 0);
          }
          if (state.y > 50)
          { // going up
            machine->updatekey(JOYK_UP, 1);
          }
          else if (state.y < -50)
          { // going down
            machine->updatekey(JOYK_DOWN, 1);
          }
          else
          {
            machine->updatekey(JOYK_UP, 0);
            machine->updatekey(JOYK_DOWN, 0);
          }
          if (state.z || state.c)
          {
            machine->updatekey(JOYK_FIRE, 1);
          }
          else
          {
            machine->updatekey(JOYK_FIRE, 0);
          }
        }
        break;
        }
      }
      else
      {
        Serial.printf("no data :(\n");
      }
      if (millis() - frame_millis > 1000)
      {
        Serial.printf("Executed at %.2FMHz cycles, frame rate=%d\n", c / 1000000.0, frames);
        frames = 0;
        frame_millis = millis();
        c = 0;
      }
    }
  }
}

void z80Runner(void *pvParameter)
{
  int8_t audioBuffer[400];
  // int lastMillis = 0;
  uint8_t *attrBase = machine->mem.p + machine->mem.vo[machine->hwopt.videopage] + 0x1800;
  while (1)
  {
    // run for 1/50th of a second - (400*175)/3.5E6MHz
    for (int i = 0; i < 400; i++)
    {
      // handle port FF for the border and flyback
      if (i < 48 || i > 192 + 48 + 48)
      {
        machine->hwopt.portFF = 0xFF;
      }
      else
      {
        // otherwise we need to populate it with the correct attribute color
        uint8_t attr = *(attrBase + 32 * (i - 48) / 8);
        machine->hwopt.portFF = attr;
      }
      // when we have finished the screen then trigger an interrupt
      if (i == 192 + 48 + 48)
      {
        machine->interrupt();
      }
      // run for 175 cucles - this matches our audio output rate of 20KHz (175/3.5E6MHz = 1/20KHz)
      c+=175;
      machine->runForCycles(175);
      if (machine->hwopt.SoundBits != 0)
      {
        audioBuffer[i] = 20;
      }
      else
      {
        audioBuffer[i] = 0;
      }
    }
    // draw a frame
    uint32_t evt = 0;
    xQueueSend(frameRenderTimerQueue, &evt, portMAX_DELAY);
    // write the audio buffer to the I2S device - this will block if the buffer is full which will control our frame rate 400/20KHz = 1/50th of a second
    audioOutput->write(audioBuffer, 400);
  }
}

void setup(void)
{
  Serial.begin(115200);

#ifdef TFT_POWER
  // turn on the TFT
  pinMode(TFT_POWER, OUTPUT);
  digitalWrite(TFT_POWER, LOW);
#endif
  // wait for the serial monitor to connect
  for (int i = 0; i < 3; i++)
  {
    delay(1000);
    Serial.printf("Waiting %i\n", i);
  }
  Serial.printf("Boot!\n");
#ifdef SPK_MODE
  pinMode(SPK_MODE, OUTPUT);
  digitalWrite(SPK_MODE, HIGH);
#endif
#ifdef USE_DAC_AUDIO
  audioOutput = new DACOutput(I2S_NUM_0);
#endif
#ifdef BUZZER_GPIO_NUM
  audioOutput = new BuzzerOutput(BUZZER_GPIO_NUM);
#endif
#ifdef PDM_GPIO_NUM
  // i2s speaker pins
  i2s_pin_config_t i2s_speaker_pins = {
      .bck_io_num = I2S_PIN_NO_CHANGE,
      .ws_io_num = GPIO_NUM_0,
      .data_out_num = PDM_GPIO_NUM,
      .data_in_num = I2S_PIN_NO_CHANGE};
  audioOutput = new PDMOutput(I2S_NUM_0, i2s_speaker_pins);
#endif
#ifdef I2S_SPEAKER_SERIAL_CLOCK
#ifdef SPK_MODE
  pinMode(SPK_MODE, OUTPUT);
  digitalWrite(SPK_MODE, HIGH);
#endif
  // i2s speaker pins
  i2s_pin_config_t i2s_speaker_pins = {
      .bck_io_num = I2S_SPEAKER_SERIAL_CLOCK,
      .ws_io_num = I2S_SPEAKER_LEFT_RIGHT_CLOCK,
      .data_out_num = I2S_SPEAKER_SERIAL_DATA,
      .data_in_num = I2S_PIN_NO_CHANGE};

  audioOutput = new I2SOutput(I2S_NUM_1, i2s_speaker_pins);
#endif
  audioOutput->start(20000);
  // Initialize SPIFFS
  if (!SPIFFS.begin(true))
  {
    Serial.printf("An Error has occurred while mounting SPIFFS\n");
    return;
  }

  Serial.println("Border Width: " + String(borderWidth));
  Serial.println("Border Height: " + String(borderHeight));

#ifdef NUNCHUK_CLOCK
  if (wii_i2c_init(0, NUNCHUK_DATA, NUNCHUK_CLOCK) != 0)
  {
    Serial.printf("ERROR initializing wii i2c controller\n");
  }
  else
  {
    const unsigned char *ident = wii_i2c_read_ident();
    if (!ident)
    {
      Serial.printf("no ident :(\n");
    }
    else
    {
      controller_type = wii_i2c_decode_ident(ident);
      switch (controller_type)
      {
      case WII_I2C_IDENT_NUNCHUK:
        Serial.printf("-> nunchuk detected\n");
        break;
      case WII_I2C_IDENT_CLASSIC:
        Serial.printf("-> classic controller detected\n");
        break;
      default:
        Serial.printf("-> unknown controller detected: 0x%06x\n", controller_type);
        break;
      }
    }
  }
#endif

  tft.begin();
#ifdef USE_DMA
  tft.initDMA(); // Initialise the DMA engine
#endif
  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK);
  Serial.printf("Total heap: ");
  Serial.println(ESP.getHeapSize());
  Serial.printf("\n");
  Serial.printf("Free heap: ");
  Serial.println(ESP.getFreeHeap());
  Serial.printf("\n");
  Serial.printf("Total PSRAM: ");
  Serial.println(ESP.getPsramSize());
  Serial.printf("\n");
  Serial.printf("Free PSRAM: ");
  Serial.println(ESP.getFreePsram());
  Serial.printf("\n");

  frameBuffer = (uint16_t *)malloc(256 * 192 * sizeof(uint16_t));
  if (frameBuffer == NULL)
  {
    Serial.printf("Error! memory not allocated for screenbuffer.\n");
    delay(10000);
  }
  memset(frameBuffer, 0, 256 * 192 * sizeof(uint16_t));

  Serial.printf("\nDespues\n");
  Serial.printf("Total heap: ");
  Serial.println(ESP.getHeapSize());
  Serial.printf("\n");
  Serial.printf("Free heap: ");
  Serial.println(ESP.getFreeHeap());
  Serial.printf("\n");
  Serial.printf("Total PSRAM: ");
  Serial.println(ESP.getPsramSize());
  Serial.printf("\n");
  Serial.printf("Free PSRAM: ");
  Serial.println(ESP.getFreePsram());
  Serial.printf("\n");

  machine = new ZXSpectrum();
  machine->reset();
  Serial.printf("Z80 Initialization completed\n");

  machine->init_spectrum(SPECMDL_48K, "/48.rom");
  machine->reset_spectrum(machine->z80Regs);
  // Load_SNA(&z80Regs, "/manic.sna");
  //  Load_SNA(&z80Regs,"/t1.sna");
  Load_SNA(machine, "/elite.sna");
  Serial.printf("Entrando en el loop\n");
  // cpuRunTimerQueue = xQueueCreate(10, sizeof(uint32_t));
  frameRenderTimerQueue = xQueueCreate(10, sizeof(uint32_t));
  // tasks to do the work
  xTaskCreatePinnedToCore(drawDisplay, "drawDisplay", 16384, NULL, 1, NULL, 1);
  // use the I2S output to control the frame rate
  xTaskCreatePinnedToCore(z80Runner, "z80Runner", 16384, NULL, 5, NULL, 0);
}

void show_nunchuk(const unsigned char *data)
{
  wii_i2c_nunchuk_state state;
  wii_i2c_decode_nunchuk(data, &state);

  Serial.printf("a = (%5d,%5d,%5d)\n", state.acc_x, state.acc_y, state.acc_z);
  Serial.printf("d = (%5d,%5d)\n", state.x, state.y);
  Serial.printf("c=%d, z=%d\n", state.c, state.z);
}

void show_classic(const unsigned char *data)
{
  wii_i2c_classic_state state;
  wii_i2c_decode_classic(data, &state);

  Serial.printf("lx,ly = (%3d,%3d)\n", state.lx, state.ly);
  Serial.printf("rx,ry = (%3d,%3d)\n", state.rx, state.ry);
  Serial.printf("a lt,rt = (%3d,%3d)\n", state.a_lt, state.a_rt);
  Serial.printf("d lt,rt = (%d,%d)\n", state.d_lt, state.d_rt);
  Serial.printf("a,b,x,y = (%d,%d,%d,%d)\n", state.a, state.b, state.x, state.y);
  Serial.printf("up, down, left, right = (%d,%d,%d,%d)\n", state.up, state.down, state.left, state.right);
  Serial.printf("home, plus, minus = (%d,%d,%d)\n", state.home, state.plus, state.minus);
  Serial.printf("zl, zr = (%d,%d)\n", state.zl, state.zr);
}

int elapsedTime = 0;
void loop(void)
{
  vTaskDelay(10);
  elapsedTime += 10;
  if (elapsedTime == 1000)
  {
    Serial.println("Sending N");
    machine->updatekey(SPECKEY_N, 1);
    vTaskDelay(200);
    machine->updatekey(SPECKEY_N, 0);
  }
  if (elapsedTime == 2000)
  {
    Serial.println("Sending SPACE");
    machine->updatekey(SPECKEY_SPACE, 1);
    vTaskDelay(200);
    machine->updatekey(SPECKEY_SPACE, 0);
  }
  if (elapsedTime == 3000)
  {
    Serial.println("Sending 1");
    machine->updatekey(SPECKEY_1, 1);
    vTaskDelay(200);
    machine->updatekey(SPECKEY_1, 0);
  }
}
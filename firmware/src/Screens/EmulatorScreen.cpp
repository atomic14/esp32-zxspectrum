#include <Arduino.h>
#include "../TFT/TFTDisplay.h"
#include "../Emulator/spectrum.h"
#include "../Emulator/snaps.h"
#include "../AudioOutput/AudioOutput.h"
#include "../Files/Files.h"
#include "EmulatorScreen.h"
#include "NavigationStack.h"
#include "SaveSnapshotScreen.h"
#include "../TZX/ZXSpectrumTapeListener.h"
#include "../TZX/DummyListener.h"
#include "../TZX/tzx_cas.h"
#include "utils.h"

const int screenWidth = TFT_WIDTH;
const int screenHeight = TFT_HEIGHT;
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
  if (!emulatorScreen->isLoading)
  {
    // do the border
    uint8_t borderColor = machine->hwopt.BorderColor & B00000111;
    uint16_t tftColor = specpal565[borderColor];
    // swap the byte order
    tftColor = (tftColor >> 8) | (tftColor << 8);
    if (tftColor != lastBorderColor || emulatorScreen->firstDraw)
    {
      // do the border with some simple rects - no need to push pixels for a solid color
      tft.fillRect(0, 0, screenWidth, borderHeight, tftColor);
      tft.fillRect(0, screenHeight - borderHeight, screenWidth, borderHeight, tftColor);
      tft.fillRect(0, borderHeight, borderWidth, screenHeight - borderHeight, tftColor);
      tft.fillRect(screenWidth - borderWidth, borderHeight, borderWidth, screenHeight - borderHeight, tftColor);
      lastBorderColor = tftColor;
    }
  } else {
    int position = emulatorScreen->loadProgress * screenWidth / 100;
    tft.fillRect(position, 0, screenWidth - position, 8, TFT_BLACK);
    tft.fillRect(0, 0, position, 8, TFT_GREEN);
    for(int borderPos = 8; borderPos < 240; borderPos++) {
      uint16_t borderColor = emulatorScreen->currentBorderColors[borderPos];
      if (emulatorScreen->drawnBorderColors[borderPos] != borderColor) {
        emulatorScreen->drawnBorderColors[borderPos] = borderColor;
        // draw the border
        if (borderPos < borderHeight || borderPos >= screenHeight - borderHeight) {
          tft.drawFastHLine(0, borderPos, screenWidth, borderColor);
        } else {
          tft.drawFastHLine(0, borderPos, borderWidth, borderColor);
          tft.drawFastHLine(screenWidth - borderWidth, borderPos, borderWidth, borderColor);
        }
      }
    }
  }
  // do the pixels
  uint8_t *attrBase = emulatorScreen->currentScreenBuffer + 0x1800;
  uint8_t *pixelBase = emulatorScreen->currentScreenBuffer;
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
      if ((attr & B10000000) != 0 && flashTimer < 16)
      {
        // we are flashing we need to swap the ink and paper colors
        uint8_t temp = inkColor;
        inkColor = paperColor;
        paperColor = temp;
        // update the attribute with the new colors - this makes our dirty check work
        attr = (attr & B11000000) | (inkColor & B00000111) | ((paperColor << 3) & B00111000);
      }
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
        uint16_t *pixelAddress = emulatorScreen->pixelBuffer + 256 * y + attrX * 8;
        for (int x = 0; x < 8; x++)
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
        if (y % 2 == 0)
        {
          dstY++;
          for (int x = 0; x < 384; x++)
          {
            scaledBuffer[dstY * 384 + x] = scaledBuffer[(dstY - 1) * 384 + x];
          }
        }
        dstY++;
      }
      tft.setWindow(borderWidth, borderHeight + attrY * 12, borderWidth + 383, borderHeight + attrY * 12 + 11);
      tft.pushPixels(scaledBuffer, 384 * 12);
#else
      tft.setWindow(borderWidth, borderHeight + attrY * 8, borderWidth + 255, borderHeight + attrY * 8 + 7);
      tft.pushPixels(emulatorScreen->pixelBuffer, 256 * 8);
#endif
    }
  }
  tft.endWrite();
  emulatorScreen->drawReady = true;
  emulatorScreen->firstDraw = false;
  emulatorScreen->frameCount++;
  flashTimer++;
  if (flashTimer >= 32)
  {
    flashTimer = 0;
  }
}

void drawDisplay(void *pvParameters)
{
  EmulatorScreen *emulatorScreen = (EmulatorScreen *)pvParameters;
  while (1)
  {
    if (!emulatorScreen->isRunning && !emulatorScreen->isLoading)
    {
      vTaskDelay(100 / portTICK_PERIOD_MS);
      continue;
    }
    if (xSemaphoreTake(emulatorScreen->m_displaySemaphore, portMAX_DELAY))
    {
      drawScreen(emulatorScreen);
    }
    if (emulatorScreen->isRunning && digitalRead(0) == LOW)
    {
      emulatorScreen->pause();
      emulatorScreen->showSaveSnapshotScreen();
    }
  }
}

void z80Runner(void *pvParameter)
{
  EmulatorScreen *emulatorScreen = (EmulatorScreen *)pvParameter;
  unsigned long lastTime = millis();
  while (1)
  {
    if (emulatorScreen->isRunning)
    {
      emulatorScreen->cycleCount += emulatorScreen->machine->runForFrame(emulatorScreen->m_audioOutput, emulatorScreen->audioFile);
      // make a copy of the current screen
      if (emulatorScreen->drawReady)
      {
        emulatorScreen->drawReady = false;
        memcpy(emulatorScreen->currentScreenBuffer, emulatorScreen->machine->mem.currentScreen, 6912);
        xSemaphoreGive(emulatorScreen->m_displaySemaphore);
      }
      // uint32_t evt = 0;
      // xQueueSend(emulatorScreen->frameRenderTimerQueue, &evt, 0);
      // drawDisplay(emulatorScreen);
      // log out some stats
      unsigned long currentTime = millis();
      unsigned long elapsed = currentTime - lastTime;
      if (elapsed > 1000)
      {
        lastTime = currentTime;
        float cycles = emulatorScreen->cycleCount / (elapsed * 1000.0);
        float fps = emulatorScreen->frameCount / (elapsed / 1000.0);
        Serial.printf("Executed at %.3FMHz cycles, frame rate=%.2f\n", cycles, fps);
        emulatorScreen->frameCount = 0;
        emulatorScreen->cycleCount = 0;
      }
    }
    else
    {
      vTaskDelay(100 / portTICK_PERIOD_MS);
    }
  }
}

EmulatorScreen::EmulatorScreen(TFTDisplay &tft, AudioOutput *audioOutput) : Screen(tft, audioOutput)
{
  pixelBuffer = (uint16_t *)malloc(screenWidth * 8 * sizeof(uint16_t));
  screenBuffer = (uint8_t *)malloc(6912);
  if (screenBuffer == NULL)
  {
    Serial.println("Failed to allocate screen buffer");
  }
  memset(screenBuffer, 0, 6912);
  currentScreenBuffer = (uint8_t *)malloc(6912);
  if (currentScreenBuffer == NULL)
  {
    Serial.println("Failed to allocate current screen buffer");
  }
  memset(currentScreenBuffer, 0, 6912);
  m_displaySemaphore = xSemaphoreCreateBinary();

  pinMode(0, INPUT_PULLUP);
}

void EmulatorScreen::loadGPIOTape()
{
  ScopeGuard guard([&](){
    isLoading = false;
    if (m_audioOutput) m_audioOutput->resume();
  });
  if (m_audioOutput) {
    m_audioOutput->pause();
  }
  isLoading = true;
  for (int i = 0; i < 200; i++)
  {
    machine->runForFrame(nullptr, nullptr);
  }
  triggerDraw();
  if (machine->hwopt.hw_model == SPECMDL_48K)
  {
    tapKey(SPECKEY_J);
    machine->updatekey(SPECKEY_SYMB, 1);
    tapKey(SPECKEY_P);
    tapKey(SPECKEY_P);
    machine->updatekey(SPECKEY_SYMB, 0);
    tapKey(SPECKEY_ENTER);
  } 
  else
  {
    // 128K the tape loader is first in the menu
    tapKey(SPECKEY_ENTER);
  }
  triggerDraw();
  GPIOTapeLoader *gpioTapeLoader = new GPIOTapeLoader(15625);
  gpioTapeLoader->start();
  int borderPos = 0;
  uint16_t borderColors[240] = {0x18C6};
  float ave = 2048;
  while (1)
  {
    uint16_t sample = gpioTapeLoader->readSample();
    ave = (ave * 0.999) + (sample * 0.001);
    if (sample > ave)
    {
      machine->setMicHigh();
    }
    else
    {
      machine->setMicLow();
    }
    machine->runForCycles(224);
    uint16_t borderColor = specpal565[machine->hwopt.BorderColor & B00000111];
    borderColors[borderPos % 240] = borderColor;
    borderPos++;
    if (borderPos == 240)
    {
      memcpy(currentBorderColors, borderColors, 240 * sizeof(uint16_t));
      triggerDraw();
    }
    if (borderPos == 240 * 10) {
      vTaskDelay(1);
      borderPos = 0;
    }
    // read pin 0 and if it is low then we are done
    if (digitalRead(0) == LOW)
    {
      gpioTapeLoader->stop();
      break;
    }
  }
}

void EmulatorScreen::tapKey(SpecKeys key) {
  machine->updatekey(key, 1);
  for (int i = 0; i < 10; i++)
  {
    machine->runForFrame(nullptr, nullptr);
  }
  machine->updatekey(key, 0);
  for (int i = 0; i < 10; i++)
  {
    machine->runForFrame(nullptr, nullptr);
  }
}

void EmulatorScreen::triggerDraw() {
  if (drawReady)
  {
    drawReady = false;
    memcpy(currentScreenBuffer, machine->mem.currentScreen, 6912);
    xSemaphoreGive(m_displaySemaphore);
  }
}

void EmulatorScreen::loadTape(std::string filename)
{
  ScopeGuard guard([&]()
  {
    isLoading = false;
    if (m_audioOutput) m_audioOutput->resume(); 
  });
  // stop audio playback
  if (m_audioOutput) 
  {
    m_audioOutput->pause();
  }
  uint64_t startTime = get_usecs();
  isLoading = true;
  Serial.printf("Loading tape %s\n", filename.c_str());
  // go into tape loading mode
  for (int i = 0; i < 200; i++)
  {
    machine->runForFrame(nullptr, nullptr);
  }
  triggerDraw();
  Serial.printf("Pressing enter\n");
  tapKey(SPECKEY_ENTER);
  triggerDraw();
  Serial.printf("Loading tape file\n");
  FILE *fp = fopen(filename.c_str(), "rb");
  if (fp == NULL)
  {
    Serial.println("Error: Could not open file.");
    std::cout << "Error: Could not open file." << std::endl;
    return;
  }
  fseek(fp, 0, SEEK_END);
  long file_size = ftell(fp);
  fseek(fp, 0, SEEK_SET);
  Serial.printf("File size %d\n", file_size);
  uint8_t *tzx_data = (uint8_t *)ps_malloc(file_size);
  if (!tzx_data)
  {
    Serial.println("Error: Could not allocate memory.");
    return;
  }
  fread(tzx_data, 1, file_size, fp);
  fclose(fp);
  // load the tape
  TzxCas tzxCas;
  DummyListener *dummyListener = new DummyListener();
  dummyListener->start();
  if (filename.find(".tap") != std::string::npos || filename.find(".TAP") != std::string::npos)
  {
    tzxCas.load_tap(dummyListener, tzx_data, file_size);
  }
  else
  {
    tzxCas.load_tzx(dummyListener, tzx_data, file_size);
  }
  dummyListener->finish();
  uint64_t totalTicks = dummyListener->getTotalTicks();
  Serial.printf("Total cycles: %lld\n", dummyListener->getTotalTicks());
  delete dummyListener;
  int count = 0;
  uint16_t borderColors[240] = {0x18C6};
  int borderPos = 0;
  ZXSpectrumTapeListener *listener = new ZXSpectrumTapeListener(machine, [&](uint64_t progress) {
        // approximate the border position - not very accutare but good enough
        // get the border color
        borderColors[borderPos] = specpal565[machine->hwopt.BorderColor & B00000111];
        borderPos++;
        count++;
        if (borderPos == 240) {
          borderPos = 0;
          memcpy(currentBorderColors, borderColors, 240 * sizeof(uint16_t));
          triggerDraw();
        }
        if (count % 4000 == 0) {
          float machineTime = (float) listener->getTotalTicks() / 3500000.0f;
          float wallTime = (float) (get_usecs() - startTime) / 1000000.0f;
          Serial.printf("Total execution time: %fs\n", (float) listener->getTotalExecutionTime() / 1000000.0f);
          Serial.printf("Total machine time: %f\n", machineTime);
          Serial.printf("Wall Clock time: %fs\n", wallTime);
          Serial.printf("Speed Up: %f\n",  machineTime/wallTime);
          Serial.printf("Progress: %lld\n", progress * 100 / totalTicks);
          // draw a progreess bar
          loadProgress = progress * 100 / totalTicks;
          vTaskDelay(1);
        }
});
  listener->start();
  if (filename.find(".tap") != std::string::npos || filename.find(".TAP") != std::string::npos)
  {
    Serial.printf("Loading tap file\n");
    tzxCas.load_tap(listener, tzx_data, file_size);
  }
  else
  {
    Serial.printf("Loading tzx file\n");
    tzxCas.load_tzx(listener, tzx_data, file_size);
  }
  Serial.printf("Tape loaded\n");
  listener->finish();
  Serial.printf("*********************");
  Serial.printf("Total execution time: %lld\n", listener->getTotalExecutionTime());
  Serial.printf("Total cycles: %lld\n", listener->getTotalTicks());
  Serial.printf("*********************");
  free(tzx_data);
  delete listener;
}

void EmulatorScreen::run(std::string filename)
{
  m_tft.fillScreen(TFT_BLACK);
  xTaskCreatePinnedToCore(drawDisplay, "drawDisplay", 8192, this, 1, NULL, 1);
  // audioFile = fopen("/fs/audio.raw", "wb");
  firstDraw = true;
  auto bl = BusyLight();
  memset(pixelBuffer, 0, screenWidth * 8 * sizeof(uint16_t));
  memset(screenBuffer, 0, 6192);
  machine = new ZXSpectrum();
  machine->reset();
  // check for tap or tpz files
  std::string ext = filename.substr(filename.find_last_of(".") + 1);
  std::transform(ext.begin(), ext.end(), ext.begin(),
                 [](unsigned char c)
                 { return std::tolower(c); });
  if (ext == "tap" || ext == "tzx")
  {
    machine->init_spectrum(SPECMDL_128K);
    machine->reset_spectrum(machine->z80Regs);
    loadTape(filename.c_str());
  }
  else
  {
    // generic loading of z80 and sna files
    machine->init_spectrum(SPECMDL_48K);
    machine->reset_spectrum(machine->z80Regs);
    Load(machine, filename.c_str());
  }
  // tasks to do the work
  Serial.println("Starting tasks\n");
  isRunning = true;
  xTaskCreatePinnedToCore(z80Runner, "z80Runner", 8192, this, 5, NULL, 0);
}

void EmulatorScreen::runGPIOTape()
{
  m_tft.fillScreen(TFT_BLACK);
  xTaskCreatePinnedToCore(drawDisplay, "drawDisplay", 8192, this, 1, NULL, 1);
  firstDraw = true;
  auto bl = BusyLight();
  memset(pixelBuffer, 0, screenWidth * 8 * sizeof(uint16_t));
  memset(screenBuffer, 0, 6192);
  machine = new ZXSpectrum();
  machine->reset();
  machine->init_spectrum(SPECMDL_48K);
  machine->reset_spectrum(machine->z80Regs);
  // start loading the tape
  loadGPIOTape();
  // tasks to do the work
  Serial.println("Starting tasks\n");
  isRunning = true;
  xTaskCreatePinnedToCore(z80Runner, "z80Runner", 8192, this, 5, NULL, 0);
}

void EmulatorScreen::run48K()
{
  m_tft.fillScreen(TFT_BLACK);
  xTaskCreatePinnedToCore(drawDisplay, "drawDisplay", 8192, this, 1, NULL, 1);

  memset(pixelBuffer, 0, screenWidth * 8 * sizeof(uint16_t));
  memset(screenBuffer, 0, 6192);
  machine = new ZXSpectrum();
  machine->reset();
  machine->init_spectrum(SPECMDL_48K);
  machine->reset_spectrum(machine->z80Regs);
  m_tft.fillScreen(TFT_WHITE);
  firstDraw = true;
  isRunning = true;
  // tasks to do the work
  xTaskCreatePinnedToCore(z80Runner, "z80Runner", 8192, this, 5, NULL, 0);
}

void EmulatorScreen::run128K()
{
  m_tft.fillScreen(TFT_BLACK);
  xTaskCreatePinnedToCore(drawDisplay, "drawDisplay", 8192, this, 1, NULL, 1);
  memset(pixelBuffer, 0, screenWidth * 8 * sizeof(uint16_t));
  memset(screenBuffer, 0, 6192);
  machine = new ZXSpectrum();
  machine->reset();
  machine->init_spectrum(SPECMDL_128K);
  machine->reset_spectrum(machine->z80Regs);
  m_tft.fillScreen(TFT_WHITE);
  firstDraw = true;
  isRunning = true;
  // tasks to do the work
  xTaskCreatePinnedToCore(z80Runner, "z80Runner", 8192, this, 5, NULL, 0);
}

void EmulatorScreen::pause()
{
  isRunning = false;
}

void EmulatorScreen::resume()
{
  // trigger a complete screen redraw
  firstDraw = true;
  // start running again
  isRunning = true;
}

void EmulatorScreen::updatekey(SpecKeys key, uint8_t state)
{
  if (key == SPECKEY_0)
  {
    if (audioFile)
    {
      isRunning = false;
      vTaskDelay(1000 / portTICK_PERIOD_MS);
      fclose(audioFile);
      audioFile = NULL;
      Serial.printf("Audio file closed\n");
    }
  }
  if (isRunning)
  {
    machine->updatekey(key, state);
  }
}

void EmulatorScreen::showSaveSnapshotScreen()
{
  m_navigationStack->push(new SaveSnapshotScreen(m_tft, m_audioOutput, machine));
}

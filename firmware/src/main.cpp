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
#include "Serial.h"
#include <esp_err.h>
#include <map>
#include <vector>
#include <sstream>
#include "SPI.h"
#include "AudioOutput/I2SOutput.h"
#include "AudioOutput/PDMOutput.h"
#include "AudioOutput/DACOutput.h"
#include "AudioOutput/BuzzerOutput.h"
#include "Emulator/snaps.h"
#include "Emulator/spectrum.h"
#include "Files/Files.h"
#include "Screens/NavigationStack.h"
#include "Screens/MainMenuScreen.h"
#include "Input/SerialKeyboard.h"
#include "Input/Nunchuck.h"
#include "Input/AdafruitSeeSaw.h"
#include "TFT/TFTDisplay.h"
#include "TFT/ST7789.h"
#include "TFT/ILI9341.h"
#ifdef TOUCH_KEYBOARD
#include "Input/TouchKeyboard.h"
#endif
#ifdef TOUCH_KEYBOARD_V2
#include "Input/TouchKeyboardV2.h"
#endif

const char *MOUNT_POINT = "/fs";

void setup(void)
{
    // Files
#ifdef USE_SDCARD
#ifdef USE_SDIO
  SDCard *fileSystem = new SDCard(MOUNT_POINT, SD_CARD_CLK, SD_CARD_CMD, SD_CARD_D0, SD_CARD_D1, SD_CARD_D2, SD_CARD_D3);
  IFiles *files = new FilesImplementation<SDCard>(fileSystem);
  setupUSB(fileSystem);
#else
  SDCard *fileSystem = new SDCard(MOUNT_POINT, SD_CARD_MISO, SD_CARD_MOSI, SD_CARD_CLK, SD_CARD_CS);
  IFiles *files = new FilesImplementation<SDCard>(fileSystem);
#endif
  Serial.begin(115200);
  // for(int i = 0; i < 5; i++) {
  //   BusyLight bl;
  //   vTaskDelay(pdMS_TO_TICKS(1000));
  //   Serial.println("Booting...");
  // }
  // print out avialable ram
  Serial.printf("Free heap: %d\n", ESP.getFreeHeap());
  Serial.printf("Free PSRAM: %d\n", ESP.getFreePsram());
  #ifdef POWER_PIN
  pinMode(POWER_PIN, OUTPUT);
  digitalWrite(POWER_PIN, POWER_PIN_ON);
  vTaskDelay(100);
  #endif
  Serial.println("Starting up");
  // navigation stack
  NavigationStack *navigationStack = new NavigationStack();
  // Audio output
#ifdef USE_DAC_AUDIO
  AudioOutput *audioOutput = new DACOutput(I2S_NUM_0);
#endif
#ifdef BUZZER_GPIO_NUM
  AudioOutput *audioOutput = new BuzzerOutput(BUZZER_GPIO_NUM);
#endif
#ifdef PDM_GPIO_NUM
  // i2s speaker pins
  i2s_pin_config_t i2s_speaker_pins = {
      .bck_io_num = I2S_PIN_NO_CHANGE,
      .ws_io_num = GPIO_NUM_0,
      .data_out_num = BUZZER_GPIO_NUM,
      .data_in_num = I2S_PIN_NO_CHANGE};
  AudioOutput *audioOutput = new PDMOutput(I2S_NUM_0, i2s_speaker_pins);
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
  AudioOutput *audioOutput = new I2SOutput(I2S_NUM_1, i2s_speaker_pins);
#endif
#ifdef TOUCH_KEYBOARD
  TouchKeyboard *touchKeyboard = new TouchKeyboard(
      [&](SpecKeys key, bool down)
      {
        {
          navigationStack->updateKey(key, down);
        }
      });
  touchKeyboard->calibrate();
  touchKeyboard->start();
#endif
#ifdef TOUCH_KEYBOARD_V2
  TouchKeyboardV2 *touchKeyboard = new TouchKeyboardV2(
      [&](SpecKeys key, bool down) 
      { navigationStack->updateKey(key, down); },
      [&](SpecKeys key)
      { navigationStack->pressKey(key); });
  touchKeyboard->start();
#endif
  if (audioOutput) {
    audioOutput->start(15625);
  }
  #ifdef TFT_ST7789
  TFTDisplay *tft = new ST7789(TFT_MOSI, TFT_SCLK, TFT_CS, TFT_DC, TFT_RST, TFT_BL, TFT_WIDTH, TFT_HEIGHT);
  #endif
  #ifdef TFT_ILI9341
  TFTDisplay *tft = new ILI9341(TFT_MOSI, TFT_SCLK, TFT_CS, TFT_DC, TFT_RST, TFT_BL, TFT_WIDTH, TFT_HEIGHT);
  #endif
#else
  Flash *fileSystem = new Flash(MOUNT_POINT);
  Files<Flash> *files = new Files<Flash>(fileSystem);
#endif
  // create the directory structure
  files->createDirectory("/snapshots");
  MainMenuScreen menuPicker(*tft, audioOutput, files);
  navigationStack->push(&menuPicker);
  // start off the keyboard and feed keys into the active scene
  SerialKeyboard *keyboard = new SerialKeyboard([&](SpecKeys key, bool down)
                                                { navigationStack->updateKey(key, down); if (down) { navigationStack->pressKey(key); } });

// start up the nunchuk controller and feed events into the active screen
#ifdef NUNCHUK_CLOCK
   Nunchuck *nunchuck = new Nunchuck([&](SpecKeys key, bool down)
                                    { navigationStack->updateKey(key, down); },
                                    [&](SpecKeys key)
                                    { navigationStack->pressKey(key); },
                                    NUNCHUK_CLOCK, NUNCHUK_DATA);
#endif
#ifdef SEESAW_CLOCK
  AdafruitSeeSaw *seeSaw = new AdafruitSeeSaw([&](SpecKeys key, bool down)
                                    { navigationStack->updateKey(key, down); },
                                    [&](SpecKeys key)
                                    { navigationStack->pressKey(key); });
  seeSaw->begin(SEESAW_DATA, SEESAW_CLOCK);
#endif

  Serial.println("Running on core: " + String(xPortGetCoreID()));
  // use the boot pin to open the emulator menu
  pinMode(0, INPUT_PULLUP);
  // just keep running
  bool bootButtonWasPressed = false;
  while (true)
  {
    vTaskDelay(100 / portTICK_PERIOD_MS);
    if (digitalRead(0) == LOW)
    {
      navigationStack->updateKey(SPECKEY_MENU, true);
    }
    else if (bootButtonWasPressed)
    {
      navigationStack->updateKey(SPECKEY_MENU, false);
      navigationStack->pressKey(SPECKEY_MENU);
      bootButtonWasPressed = false;
    }
  }
}

unsigned long frame_millis;
void loop()
{
}

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
#include "Screens/PickerScreen.h"
#include "Screens/EmulatorScreen.h"
#include "Screens/ErrorScreen.h"
#include "Screens/VideoPlayerScreen.h"
#include "Input/SerialKeyboard.h"
#include "Input/Nunchuck.h"
#include "TFT/TFTDisplay.h"
#include "TFT/TFTeSPIWrapper.h"
#include "TFT/ST7789.h"
#ifdef TOUCH_KEYBOARD
#include "Input/TouchKeyboard.h"
#endif
#ifdef TOUCH_KEYBOARD_V2
#include "Input/TouchKeyboardV2.h"
#endif

// Mode picker
class MenuItem
{
public:
  MenuItem(const std::string &title, std::function<void()> onSelect) : title(title), onSelect(onSelect) {}
  std::string getTitle() const { return title; }
  // callback for when the item is selected
  std::function<void()> onSelect;
private:
  std::string title;
};

using MenuItemPtr = std::shared_ptr<MenuItem>;
using MenuItemVector = std::vector<MenuItemPtr>;

TFTDisplay *tft = nullptr;
AudioOutput *audioOutput = nullptr;
PickerScreen<MenuItemPtr> *menuPicker = nullptr;
PickerScreen<FileInfoPtr> *gameFilePickerScreen = nullptr;
PickerScreen<FileLetterCountPtr> *gameAlphabetPicker = nullptr;
PickerScreen<FileInfoPtr> *videoFilePickerScreen = nullptr;
PickerScreen<FileLetterCountPtr> *videoAlphabetPicker = nullptr;
EmulatorScreen *emulatorScreen = nullptr;
VideoPlayerScreen *videoPlayerScreen = nullptr;
ErrorScreen *loadSDCardScreen = nullptr;
ErrorScreen *noGamesScreen = nullptr;
Screen *activeScreen = nullptr;
#ifdef USE_SDCARD
Files<SDCard> *files = nullptr;
#else
Files<Flash> *files = nullptr;
#endif
SerialKeyboard *keyboard = nullptr;
Nunchuck *nunchuck = nullptr;
#ifdef TOUCH_KEYBOARD
TouchKeyboard *touchKeyboard = nullptr;
#endif
#ifdef TOUCH_KEYBOARD_V2
TouchKeyboardV2 *touchKeyboard = nullptr;
#endif

const std::vector<std::string> gameValidExtensions = {".z80", ".sna"};
const std::vector<std::string> videoValidExtensions = {".avi"};
const char *MOUNT_POINT = "/fs";

void setup(void)
{
  Serial.begin(115200);
  // for(int i = 0; i<5; i++)
  // {
  //   Serial.print(".");
  //   delay(1000);
  // }
  Serial.println("Starting up");
  // Audio output
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
  i2s speaker pins
  i2s_pin_config_t i2s_speaker_pins = {
      .bck_io_num = I2S_PIN_NO_CHANGE,
      .ws_io_num = GPIO_NUM_0,
      .data_out_num = BUZZER_GPIO_NUM,
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
#ifdef TOUCH_KEYBOARD
  touchKeyboard = new TouchKeyboard(
    [&](SpecKeys key, bool down) {
    {
      activeScreen->updatekey(key, down);
    }
  });
  touchKeyboard->calibrate();
  touchKeyboard->start();
#endif
#ifdef TOUCH_KEYBOARD_V2
  touchKeyboard = new TouchKeyboardV2(
    [&](SpecKeys key, bool down) {
    {
      activeScreen->updatekey(key, down);
    }
  });
  touchKeyboard->start();
#endif
  audioOutput->start(15625);
  tft = new ST7789(TFT_MOSI, TFT_SCLK, TFT_CS, TFT_DC, TFT_RST, TFT_BL, 320, 240);
  // tft = new TFTeSPIWrapper();
  // Files
#ifdef USE_SDCARD
#ifdef SD_CARD_PWR
    if (SD_CARD_PWR != GPIO_NUM_NC)
    {
      pinMode(SD_CARD_PWR, OUTPUT);
      digitalWrite(SD_CARD_PWR, SD_CARD_PWR_ON);
    }
#endif
#ifdef USE_SDIO
    SDCard *fileSystem = new SDCard(MOUNT_POINT, SD_CARD_CLK, SD_CARD_CMD, SD_CARD_D0, SD_CARD_D1, SD_CARD_D2, SD_CARD_D3);
    files = new Files<SDCard>(fileSystem);
#else
    SDCard *fileSystem = new SDCard(MOUNT_POINT, SD_CARD_MISO, SD_CARD_MOSI, SD_CARD_CLK, SD_CARD_CS);
    files = new Files<SDCard>(fileSystem);
#endif
#else
    Flash *fileSystem = new Flash(MOUNT_POINT);
    files = new Files<Flash>(fileSystem);
#endif
  
  // Main menu
  MenuItemVector menuItems = {
    std::make_shared<MenuItem>("48K ZX Spectrum", [&]() {
      emulatorScreen->run48K();
      activeScreen = emulatorScreen;
      #ifdef TOUCH_KEYBOARD
      if (touchKeyboard)
      {
        touchKeyboard->setToggleMode(true);
      }
      #endif
      activeScreen->didAppear();
    }),
    std::make_shared<MenuItem>("128K ZX Spectrum", [&]() {
      emulatorScreen->run128K();
      activeScreen = emulatorScreen;
      #ifdef TOUCH_KEYBOARD
      if (touchKeyboard)
      {
        touchKeyboard->setToggleMode(true);
      }
      #endif
      activeScreen->didAppear();
    }),
    std::make_shared<MenuItem>("Games", [&]() {
      if (files->isAvailable())
      {
        // feed in the alphabetically grouped files to the alphabet picker
        FileLetterCountVector fileLetterCounts = files->getFileLetters("/", gameValidExtensions);
        gameAlphabetPicker->setItems(fileLetterCounts);
        if (fileLetterCounts.size() == 0)
        {
          activeScreen = noGamesScreen;
        }
        else
        {
          activeScreen = gameAlphabetPicker;
        }
      }
      else
      {
        activeScreen = loadSDCardScreen;
      }
      activeScreen->didAppear();
    }),
    std::make_shared<MenuItem>("Video Player", [&]() {
      if (files->isAvailable())
      {
        // feed in the alphabetically grouped files to the alphabet picker
        FileLetterCountVector fileLetterCounts = files->getFileLetters("/", videoValidExtensions);
        videoAlphabetPicker->setItems(fileLetterCounts);
        if (fileLetterCounts.size() == 0)
        {
          activeScreen = noGamesScreen;
        }
        else
        {
          activeScreen = videoAlphabetPicker;
        }
      }
      else
      {
        activeScreen = loadSDCardScreen;
      }
      activeScreen->didAppear();
    }),
  };
  // wire everythign up
  loadSDCardScreen = new ErrorScreen(
    *tft, 
    audioOutput,
    { "No SD Card", "Insert an SD Card", "to load games" },
    [&]() {
    // go back to the mode picker
    activeScreen = menuPicker;
    activeScreen->didAppear();
  });
  noGamesScreen = new ErrorScreen(
    *tft, 
    audioOutput,
    { "No games found", "on the SD Card", "add Z80 or SNA files" },
    [&]() {
    // go back to the mode picker
    activeScreen = menuPicker;
    activeScreen->didAppear();
  });
  emulatorScreen = new EmulatorScreen(*tft, audioOutput);
  videoPlayerScreen = new VideoPlayerScreen(*tft, audioOutput, [&]() {
    // go back to the mode picker
    activeScreen = menuPicker;
    activeScreen->didAppear();
  });
  menuPicker = new PickerScreen<MenuItemPtr>(*tft, audioOutput, [&](MenuItemPtr mode, int index) {
    mode->onSelect();
  }, [&]() {
    // nothing to do here - we're at the top level
  });
  // game picking
  gameAlphabetPicker = new PickerScreen<FileLetterCountPtr>(*tft, audioOutput, [&](FileLetterCountPtr entry, int index) {
    // a letter was picked - show the files for that letter
    Serial.printf("Picked letter: %s\n", entry->getLetter().c_str()), 
    gameFilePickerScreen->setItems(files->getFileStartingWithPrefix("/", entry->getLetter().c_str(), gameValidExtensions));
    activeScreen = gameFilePickerScreen;
  }, [&]() {
    // go back to the mode picker
    activeScreen = menuPicker;
    activeScreen->didAppear();
  });
  gameFilePickerScreen = new PickerScreen<FileInfoPtr>(*tft, audioOutput, [&](FileInfoPtr file, int index) {
    // a file was picked - load it into the emulator
    Serial.printf("Loading snapshot: %s\n", file->getPath().c_str());
    // switch the touch keyboard to non toggle - we don't want shift and sym-shift to be sticky
    #ifdef TOUCH_KEYBOARD
    if (touchKeyboard)
    {
      touchKeyboard->setToggleMode(false);
    }
    #endif
    emulatorScreen->run(file->getPath());
    activeScreen = emulatorScreen;
  }, [&]() {
    // go back to the alphabet picker
    activeScreen = gameAlphabetPicker;
    activeScreen->didAppear();
  });
  // video picking - TODO - can we combine this with the game picking code?
  videoAlphabetPicker = new PickerScreen<FileLetterCountPtr>(*tft, audioOutput, [&](FileLetterCountPtr entry, int index) {
    // a letter was picked - show the files for that letter
    Serial.printf("Picked letter: %s\n", entry->getLetter().c_str()), 
    videoFilePickerScreen->setItems(files->getFileStartingWithPrefix("/", entry->getLetter().c_str(), videoValidExtensions));
    activeScreen = videoFilePickerScreen;
  }, [&]() {
    // go back to the mode picker
    activeScreen = menuPicker;
    activeScreen->didAppear();
  });
  videoFilePickerScreen = new PickerScreen<FileInfoPtr>(*tft, audioOutput, [&](FileInfoPtr file, int index) {
    // a file was picked - load it into the emulator
    Serial.printf("Loading video: %s\n", file->getPath().c_str());
    activeScreen = videoPlayerScreen;
    videoPlayerScreen->play(file->getPath().c_str());
    
  }, [&]() {
    // go back to the alphabet picker
    activeScreen = gameAlphabetPicker;
    activeScreen->didAppear();
  });
  // set the mode picker to show the emulator modes
  menuPicker->setItems(menuItems);
  // start off the keyboard and feed keys into the active scene
  keyboard = new SerialKeyboard([&](SpecKeys key, bool down) {
    if (activeScreen)
    {
      activeScreen->updatekey(key, down);
    }
  });

  // start up the nunchuk controller and feed events into the active screen
  #ifdef NUNCHUK_CLOCK
  nunchuck = new Nunchuck([&](SpecKeys key, bool down) {
    if (activeScreen)
    {
      activeScreen->updatekey(key, down);
    }
  }, NUNCHUK_CLOCK, NUNCHUK_DATA);
  #endif
  // start off on the file picker screen
  activeScreen = menuPicker;
  // activeScreen = emulatorScreen;
  // activeScreen->didAppear();
  // load manic.sna
  // emulatorScreen->run("/fs/manic.sna");
  Serial.println("Running on core: " + String(xPortGetCoreID()));
}

unsigned long frame_millis;
void loop()
{
  auto ms = millis() - frame_millis;
  if (ms > 1000)
  {
    if (activeScreen == emulatorScreen)
    {
      float cycles = emulatorScreen->cycleCount / (ms * 1000.0);
      float fps = emulatorScreen->frameCount / (ms / 1000.0);
      Serial.printf("Executed at %.3FMHz cycles, frame rate=%.2f\n", cycles, fps);
      emulatorScreen->frameCount = 0;
      emulatorScreen->cycleCount = 0;
      frame_millis = millis();
    }
  }
}

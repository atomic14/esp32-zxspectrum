#pragma

#include "PickerScreen.h"
#include "../Files/Files.h"
#include "EmulatorScreen.h"

class GameFilePickerScreen : public PickerScreen<FileInfoPtr>
{
  private:
    IFiles *m_files;
  public:
      GameFilePickerScreen(TFTDisplay &tft, AudioOutput *audioOutput, IFiles *files)
      : PickerScreen("Games", tft, audioOutput), m_files(files) {}
      void onItemSelect(FileInfoPtr item, int index) {
        Serial.printf("Selected %s\n", item->getPath().c_str());
        // locate the emaulator screen if it's on the stack already
        EmulatorScreen *emulatorScreen = nullptr;
        for (int i = 0; i < m_navigationStack->stack.size(); i++) {
          emulatorScreen = dynamic_cast<EmulatorScreen *>(m_navigationStack->stack.at(i));
          if (emulatorScreen != nullptr) {
            break;
          }
        }
        // if we found the emulator screen and if we're loading a tape file then we've been triggered due to the user starting the
        // load routine from the emulator screen. We just need to tell the emulator screen to load the tape file and pop back to it
        if (emulatorScreen != nullptr && item->getExtension() == ".tap" || item->getExtension() == ".tzx") {
          Serial.println("Loading tape file into existing emulator screen");
          // pop to the emaulator screen - get a local copy of the stack as it will be set to null when we pop
          NavigationStack *navStack = m_navigationStack;
          while(navStack->stack.back() != emulatorScreen) {
            navStack->pop();
          }
          emulatorScreen->loadTape(item->getPath());
          return;
        }
        Serial.println("Starting new emulator screen");
        drawBusy();
        emulatorScreen = new EmulatorScreen(m_tft, m_audioOutput, m_files);
        // TODO - we should pick the machine to run on - 48k or 128k
        // there's no way to know from the file name or the file contents
        emulatorScreen->run(item->getPath(), models_enum::SPECMDL_48K);
        m_navigationStack->push(emulatorScreen);
      }
      void onBack() {
        m_navigationStack->pop();
      }
};


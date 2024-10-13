#pragma

#include "PickerScreen.h"
#include "../Files/Files.h"
#include "EmulatorScreen.h"

class GameFilePickerScreen : public PickerScreen<FileInfoPtr>
{
  public:
      GameFilePickerScreen(TFTDisplay &tft, AudioOutput *audioOutput)
      : PickerScreen("Games", tft, audioOutput) {}
      void onItemSelect(FileInfoPtr item, int index) {
        drawBusy();
        EmulatorScreen *emulatorScreen = new EmulatorScreen(m_tft, m_audioOutput);
        emulatorScreen->run(item->getPath());
        m_navigationStack->push(emulatorScreen);
      }
      void onBack() {
        m_navigationStack->pop();
      }
};


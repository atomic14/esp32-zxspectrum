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
        // TODO - we should pick the machine to run on - 48k or 128k
        // there's no way to know from the file name or the file contents
        emulatorScreen->run(item->getPath(), models_enum::SPECMDL_128K);
        m_navigationStack->push(emulatorScreen);
      }
      void onBack() {
        m_navigationStack->pop();
      }
};


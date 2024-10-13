#pragma

#include "VideoPlayerScreen.h"
#include "../Files/Files.h"

class VideoFilePickerScreen : public PickerScreen<FileInfoPtr>
{
  public:
      VideoFilePickerScreen(TFTDisplay &tft, AudioOutput *audioOutput)
      : PickerScreen(tft, audioOutput) {}
      void onItemSelect(FileInfoPtr item, int index) {
        VideoPlayerScreen *playerScreen = new VideoPlayerScreen(m_tft, m_audioOutput);
        playerScreen->play(item->getPath().c_str());
        m_navigationStack->push(playerScreen);
      }
};


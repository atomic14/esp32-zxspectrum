#pragma

#include "VideoPlayerScreen.h"
#include "../Files/Files.h"

class VideoFilePickerScreen : public PickerScreen<FileInfoPtr>
{
  public:
      VideoFilePickerScreen(Display &tft, HDMIDisplay *hdmiDisplay, AudioOutput *audioOutput, IFiles *files)
      : PickerScreen("Videos", tft, hdmiDisplay, audioOutput, files) {}
      void onItemSelect(FileInfoPtr item, int index) {
        VideoPlayerScreen *playerScreen = new VideoPlayerScreen(m_tft, m_hdmiDisplay, m_audioOutput, m_files);
        playerScreen->play(item->getPath().c_str());
        m_navigationStack->push(playerScreen);
      }
};


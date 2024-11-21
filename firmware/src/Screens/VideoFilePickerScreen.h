#pragma

#include "VideoPlayerScreen.h"
#include "../Files/Files.h"

class VideoFilePickerScreen : public PickerScreen<FileInfoPtr>
{
  private:
    IFiles *m_files;
  public:
      VideoFilePickerScreen(TFTDisplay &tft, AudioOutput *audioOutput, IFiles *files)
      : PickerScreen("Videos", tft, audioOutput), m_files(files) {}
      void onItemSelect(FileInfoPtr item, int index) {
        VideoPlayerScreen *playerScreen = new VideoPlayerScreen(m_tft, m_audioOutput);
        playerScreen->play(item->getPath().c_str());
        m_navigationStack->push(playerScreen);
      }
};


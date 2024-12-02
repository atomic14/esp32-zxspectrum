#pragma once

#include "../Files/Files.h"
#include "NavigationStack.h"
#include "PickerScreen.h"

template <class FilePickerScreen_T>
class AlphabetPicker : public PickerScreen<FileLetterCountPtr>
{
  private:
    IFiles *m_files;
    std::string m_path;
    std::vector<std::string> m_extensions;
  public:
    AlphabetPicker(std::string title, IFiles *files, Display &tft, HDMIDisplay *hdmiDisplay, AudioOutput *audioOutput, std::string path, std::vector<std::string> extensions) 
      : m_files(files), PickerScreen(title, tft, hdmiDisplay, audioOutput), m_path(path), m_extensions(extensions)
    {
    }
    void onItemSelect(FileLetterCountPtr item, int index) override
    {
        FilePickerScreen_T *filePickerScreen = new FilePickerScreen_T(m_tft, m_hdmiDisplay, m_audioOutput, m_files);
        drawBusy();
        filePickerScreen->setItems(m_files->getFileStartingWithPrefix(m_path.c_str(), item->getLetter().c_str(), m_extensions));
        m_navigationStack->push(filePickerScreen);
    }
};
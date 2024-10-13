#pragma once

#include "../Files/Files.h"
#include "NavigationStack.h"
#include "PickerScreen.h"

template <class Files_T, class FilePickerScreen_T>
class AlphabetPicker : public PickerScreen<FileLetterCountPtr>
{
  private:
    Files_T *m_files;
    std::string m_path;
    std::vector<std::string> m_extensions;
  public:
    AlphabetPicker(Files_T *files, TFTDisplay &tft, AudioOutput *audioOutput, std::string path, std::vector<std::string> extensions) 
      : m_files(files), PickerScreen(tft, audioOutput), m_path(path), m_extensions(extensions)
    {
    }
    void onItemSelect(FileLetterCountPtr item, int index) override
    {
        FilePickerScreen_T *filePickerScreen = new FilePickerScreen_T(m_tft, m_audioOutput);
        drawBusy();
        filePickerScreen->setItems(m_files->getFileStartingWithPrefix(m_path.c_str(), item->getLetter().c_str(), m_extensions));
        m_navigationStack->push(filePickerScreen);
    }
};
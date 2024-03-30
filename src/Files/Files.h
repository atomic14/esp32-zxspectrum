#include <dirent.h>
#include <memory>
#include <vector>
#include <string>
#include <cstring>
#include <iostream>
#include "SDCard.h"
#include "Flash.h"

std::string upcase(const std::string &str)
{
  std::string result = str;
  std::transform(result.begin(), result.end(), result.begin(), 
                 [](unsigned char c) { return std::toupper(c); });
  return result;
}

class FileInfo
{
public:
  FileInfo(const std::string &name, const std::string &path)
      : name(name), path(path) {}
  std::string getName() const { return name; }
  std::string getPath() const { return path; }

private:
  std::string name;
  std::string path;
};

using FileInfoPtr = std::shared_ptr<FileInfo>;
using FileInfoVector = std::vector<FileInfoPtr>;

class FileLetterGroup
{
public:
  FileLetterGroup(const std::string &name) : name(name) {}
  std::string getName() const { return name; }
  void setName(const std::string &name) { this->name = name; }
  void addFile(FileInfoPtr file) { files.push_back(file); }
  const FileInfoVector &getFiles() const { return files; }
  void sortFiles()
  {
    std::sort(files.begin(), files.end(), [](const FileInfoPtr &a, const FileInfoPtr &b) {
      return a->getName() < b->getName();
    });
  }
private:
  std::string name;
  FileInfoVector files;
};

using FileLetterGroupPtr = std::shared_ptr<FileLetterGroup>;
using FileLetterGroupVector = std::vector<FileLetterGroupPtr>;

class Files
{
  FileLetterGroupVector filesGroupedByLetter;
public:
  Files(const char *folder, const char *extension)
  {
#ifdef USE_SDCARD
#ifdef SD_CARD_PWR
    if (SD_CARD_PWR != GPIO_NUM_NC)
    {
      pinMode(SD_CARD_PWR, OUTPUT);
      digitalWrite(SD_CARD_PWR, SD_CARD_PWR_ON);
    }
#endif
#ifdef USE_SDIO
    SDCard *card = new SDCard(MOUNT_POINT, SD_CARD_CLK, SD_CARD_CMD, SD_CARD_D0, SD_CARD_D1, SD_CARD_D2, SD_CARD_D3);
#else
    SDCard *card = new SDCard(MOUNT_POINT, SD_CARD_MISO, SD_CARD_MOSI, SD_CARD_CLK, SD_CARD_CS);
#endif
#else
    Flash *flash = new Flash(MOUNT_POINT);
#endif
    std::string full_path = std::string(MOUNT_POINT) + "/";
    std::cout << "Listing directory: " << full_path << std::endl;

    DIR *dir = opendir(full_path.c_str());
    if (!dir)
    {
      std::cout << "Failed to open directory" << std::endl;
      return;
    }

    std::map<std::string, FileLetterGroupPtr> firstLetterFiles;
    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL)
    {
      Serial.println(ent->d_name);
      std::string filename = std::string(ent->d_name);
      bool isFile = ent->d_type == DT_REG;
      bool isVisible = filename[0] != '.';
      bool isMatchingExtension = extension == NULL || filename.substr(filename.length() - strlen(extension)) == extension;
      if (isFile && isVisible && isMatchingExtension && filename.length() > 0)
      {
        // get the first letter
        auto name = upcase(filename);
        auto letter = name.substr(0, 1);
        if (firstLetterFiles.find(letter) == firstLetterFiles.end())
        {
          firstLetterFiles[letter] = FileLetterGroupPtr(new FileLetterGroup(letter));
        }
        firstLetterFiles[letter]->addFile(FileInfoPtr(new FileInfo(name, full_path + filename)));
      }
    }
    closedir(dir);
    for (auto &entry : firstLetterFiles)
    {
      std::ostringstream oss;
      oss << entry.second->getName() << " (" << entry.second->getFiles().size() << " files)";
      entry.second->setName(oss.str());
      entry.second->sortFiles();
      filesGroupedByLetter.push_back(entry.second);
    }
  }
  const FileLetterGroupVector &getGroupedFiles() const
  {
    return filesGroupedByLetter;
  }

private:
  static constexpr const char *MOUNT_POINT = "/fs";
};

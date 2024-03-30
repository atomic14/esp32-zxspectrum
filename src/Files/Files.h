#include <dirent.h>
#include <memory>
#include <vector>
#include <string>
#include <cstring>
#include <iostream>
#include "SDCard.h"
#include "Flash.h"

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
  void addFile(FileInfoPtr file) { files.push_back(file); }
  const FileInfoVector &getFiles() const { return files; }

private:
  std::string name;
  FileInfoVector files;
};

using FileLetterGroupPtr = std::shared_ptr<FileLetterGroup>;
using FileLetterGroupVector = std::vector<FileLetterGroupPtr>;

class Files
{
public:
  Files()
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
  }
  FileInfoVector listFilePaths(const char *folder, const char *extension)
  {
    FileInfoVector files;
    std::string full_path = std::string(MOUNT_POINT) + folder;
    std::cout << "Listing directory: " << full_path << std::endl;

    DIR *dir = opendir(full_path.c_str());
    if (!dir)
    {
      std::cout << "Failed to open directory" << std::endl;
      return files; // This will return an empty vector if the directory cannot be opened
    }

    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL)
    {
      std::string filename = std::string(ent->d_name);
      bool isFile = ent->d_type == DT_REG;
      bool isVisible = filename[0] != '.';
      bool isMatchingExtension = extension == NULL || filename.substr(filename.length() - strlen(extension)) == extension;
      if (isFile && isVisible && isMatchingExtension)
      {
        files.push_back(FileInfoPtr(new FileInfo(filename, full_path + filename)));
      }
    }
    closedir(dir);

    // Sorting is removed as per request, but it could be added here if needed

    return files;
  }

private:
  static constexpr const char *MOUNT_POINT = "/fs";
};

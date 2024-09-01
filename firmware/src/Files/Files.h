#include <Arduino.h>
#include <dirent.h>
#include <algorithm>
#include <memory>
#include <vector>
#include <string>
#include <sstream>
#include <cstring>
#include <iostream>
#include "SDCard.h"
#include "Flash.h"

std::string upcase(const std::string &str)
{
  std::string result = str;
  std::transform(result.begin(), result.end(), result.begin(),
                 [](unsigned char c)
                 { return std::toupper(c); });
  return result;
}

std::string downcase(const std::string &str)
{
  std::string result = str;
  std::transform(result.begin(), result.end(), result.begin(),
                 [](unsigned char c)
                 { return std::tolower(c); });
  return result;
}

// Iterator for directory entries with optional file extension and prefix filter
class DirectoryIterator
{
public:
  using value_type = struct dirent;
  using difference_type = std::ptrdiff_t;
  using pointer = value_type *;
  using reference = value_type &;
  using iterator_category = std::input_iterator_tag;

  // Construct "end" iterator
  DirectoryIterator() : dirp(nullptr), entry(nullptr), extension(nullptr) {}

  // Construct iterator for directory path with optional file prefix and extension filter
  DirectoryIterator(const std::string &path, const char *prefix, const char *ext = nullptr) : dirp(opendir(path.c_str())), prefix(prefix), extension(ext)
  {
    if (!dirp)
    {
      throw std::runtime_error("Failed to open directory");
    }
    ++(*this); // Load first valid entry
  }

  // Prevent copy construction
  DirectoryIterator(const DirectoryIterator &other) = delete;

  // Allow move construction
  DirectoryIterator(DirectoryIterator &&other) noexcept
      : dirp(other.dirp), entry(other.entry), extension(other.extension)
  {
    other.dirp = nullptr;
    other.entry = nullptr;
  }

  ~DirectoryIterator()
  {
    if (dirp)
    {
      closedir(dirp);
    }
  }

  // Dereference operators
  pointer operator->() const { return entry; }
  reference operator*() const { return *entry; }

  // Pre-increment operator - modified to filter entries
  DirectoryIterator &operator++()
  {
    do
    {
      entry = readdir(dirp);
    } while (entry && !isValidEntry());

    if (!entry)
    { // No more valid entries
      closedir(dirp);
      dirp = nullptr;
    }
    return *this;
  }

  // Comparison operators
  bool operator==(const DirectoryIterator &other) const
  {
    return (dirp == nullptr && other.dirp == nullptr);
  }

  bool operator!=(const DirectoryIterator &other) const
  {
    return !(*this == other);
  }

private:
  DIR *dirp;
  pointer entry;
  const char *extension;
  const char *prefix;

  bool isValidEntry()
  {
    if (entry->d_type != DT_REG)
    {
      return false; // Not a regular file
    }

    std::string filename = entry->d_name;
    if (filename[0] == '.')
    {
      return false; // Hidden file
    }

    std::string lowerCaseFilename = downcase(filename);
    if (extension != nullptr)
    {
      std::string lowerCaseExtension = downcase(extension);
      if (lowerCaseFilename.length() < lowerCaseExtension.length() ||
          lowerCaseFilename.substr(lowerCaseFilename.length() - lowerCaseExtension.length()) != lowerCaseExtension)
      {
        return false; // Extension does not match
      }
    }
    if (prefix != nullptr)
    {
      std::string lowerCasePrefix = downcase(prefix);
      if (lowerCaseFilename.length() < lowerCasePrefix.length() ||
          lowerCaseFilename.substr(0, lowerCasePrefix.length()) != lowerCasePrefix)
      {
        return false; // Prefix does not match
      }
    }

    return true; // Entry is valid
  }
};

// File information class
class FileInfo
{
public:
  FileInfo(const std::string &title, const std::string &path)
      : title(title), path(path) {}
  std::string getTitle() const { return title; }
  std::string getPath() const { return path; }

private:
  std::string title;
  std::string path;
};

using FileInfoPtr = std::shared_ptr<FileInfo>;
using FileInfoVector = std::vector<FileInfoPtr>;

// File letter count class
class FileLetterCount
{
public:
  FileLetterCount(const std::string &letter, int fileCount) : letter(letter), fileCount(fileCount) {}
  std::string getTitle() const
  {
    std::stringstream title;
    title << letter << " (" << fileCount << " files)";
    return title.str();
  }
  std::string getLetter() const { return letter; }

private:
  std::string letter;
  int fileCount;
};

using FileLetterCountPtr = std::shared_ptr<FileLetterCount>;
using FileLetterCountVector = std::vector<FileLetterCountPtr>;

// Files class to list files in a directory
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

  FileLetterCountVector getFileLetters(const char *folder, const char *extension)
  {
    FileLetterCountVector fileLetters;

    std::string full_path = std::string(MOUNT_POINT) + folder;
    std::cout << "Listing directory: " << full_path << std::endl;

    std::map<std::string, int> fileCountByLetter;

    for (DirectoryIterator it(full_path, nullptr, extension); it != DirectoryIterator(); ++it)
    {
      std::string filename = it->d_name;
      std::string lowerCaseFilename = downcase(filename);
      // get the first letter
      auto name = upcase(filename);
      auto letter = name.substr(0, 1);
      if (fileCountByLetter.find(letter) == fileCountByLetter.end())
      {
        fileCountByLetter[letter] = 0;
      }
      fileCountByLetter[letter]++;
    }
    for (auto &entry : fileCountByLetter)
    {
      fileLetters.push_back(FileLetterCountPtr(new FileLetterCount(entry.first, entry.second)));
    }
    // sort the fileLetters alphabetically
    std::sort(fileLetters.begin(), fileLetters.end(), [](FileLetterCountPtr a, FileLetterCountPtr b)
              { return a->getLetter() < b->getLetter(); });
    return fileLetters;
  }

  FileInfoVector getFileStartingWithPrefix(const char *folder, const char *prefix, const char *extension)
  {
    FileInfoVector files;

    std::string full_path = std::string(MOUNT_POINT) + folder;
    std::cout << "Listing directory: " << full_path << " for files starting with " << prefix << " extension " << extension << std::endl;

    for (DirectoryIterator it(full_path, prefix, extension); it != DirectoryIterator(); ++it)
    {
      files.push_back(FileInfoPtr(new FileInfo(upcase(it->d_name), full_path + it->d_name)));
    }
    // sort the files - is this needed? Maybe they are already alphabetically sorted
    std::sort(files.begin(), files.end(), [](FileInfoPtr a, FileInfoPtr b)
              { return a->getTitle() < b->getTitle(); });
    return files;
  }

private:
  static constexpr const char *MOUNT_POINT = "/fs";
};

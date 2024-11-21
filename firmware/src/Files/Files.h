#pragma once

#include <Arduino.h>
#include "../Serial.h"
#include <dirent.h>
#include <algorithm>
#include <memory>
#include <map>
#include <vector>
#include <string>
#include <sstream>
#include <cstring>
#include <iostream>
#include "SDCard.h"
#include "Flash.h"
#include <sys/stat.h>
#include <unistd.h>

class StringUtils
{
public:
  static std::string upcase(const std::string &str)
  {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c)
                   { return std::toupper(c); });
    return result;
  }

  static std::string downcase(const std::string &str)
  {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c)
                   { return std::tolower(c); });
    return result;
  }
};

class BusyLight
{
public:
  BusyLight()
  {
    #ifdef LED_GPIO
    pinMode(LED_GPIO, OUTPUT);
    digitalWrite(LED_GPIO, HIGH);
    #endif
  }
  ~BusyLight()
  {
    #ifdef LED_GPIO
    digitalWrite(LED_GPIO, LOW);
    #endif
  }
};

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
  DirectoryIterator() : dirp(nullptr), entry(nullptr) {}

  // Construct iterator for directory path with optional file prefix and extension filter
  DirectoryIterator(const std::string &path, const char *prefix, std::vector<std::string> extensions) : dirp(opendir(path.c_str())), prefix(prefix), extensions(extensions)
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
      : dirp(other.dirp), entry(other.entry), extensions(other.extensions)
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
  std::vector<std::string> extensions;
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

    std::string lowerCaseFilename = StringUtils::downcase(filename);
    if (extensions.size() > 0)
    {
      bool validExtension = false;
      for (const std::string &extension : extensions)
      {
        std::string lowerCaseExtension = StringUtils::downcase(extension);
        if (lowerCaseFilename.length() >= lowerCaseExtension.length() &&
            lowerCaseFilename.substr(lowerCaseFilename.length() - lowerCaseExtension.length()) == lowerCaseExtension)
        {
          validExtension = true;
          break;
        }
      }
      if (!validExtension)
      {
        return false; // Extension does not match
      }
    }
    if (prefix != nullptr)
    {
      std::string lowerCasePrefix = StringUtils::downcase(prefix);
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
  std::string getExtension() const
  {
    size_t pos = title.find_last_of('.');
    if (pos != std::string::npos)
    {
      std::string extension = title.substr(pos);
      return StringUtils::downcase(extension);
    }
    return "";
  }

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
class IFiles
{
public:
  virtual bool isAvailable() = 0;
  virtual void createDirectory(const char *folder) = 0;
  virtual FileLetterCountVector getFileLetters(const char *folder, const std::vector<std::string> &extensions) = 0;
  virtual FileInfoVector getFileStartingWithPrefix(const char *folder, const char *prefix, const std::vector<std::string> &extensions) = 0;
};

template <class FileSystemT>
class FilesImplementation: public IFiles
{
private:
  FileSystemT *fileSystem;

public:
  FilesImplementation(FileSystemT *fileSystem) : fileSystem(fileSystem)
  {
  }

  bool isAvailable()
  {
    return fileSystem->isMounted();
  }

  void createDirectory(const char *folder)
  {
    auto bl = BusyLight();
    if (!fileSystem->isMounted())
    {
      return;
    }
    std::string full_path = std::string(fileSystem->mountPoint()) + folder;
    // check to see if the folder exists
    struct stat st;
    if (stat(full_path.c_str(), &st) == -1)
    {
      Serial.printf("Creating folder %s\n", full_path.c_str());
      mkdir(full_path.c_str(), 0777);
    }
    else
    {
      Serial.printf("Folder %s already exists\n", full_path.c_str());
    }
  }

  FileLetterCountVector getFileLetters(const char *folder, const std::vector<std::string> &extensions)
  {
    auto bl = BusyLight();
    FileLetterCountVector fileLetters;
    if (!fileSystem->isMounted())
    {
      return fileLetters;
    }
    std::string full_path = std::string(fileSystem->mountPoint()) + folder;
    std::cout << "Listing directory: " << full_path << std::endl;

    std::map<std::string, int> fileCountByLetter;

    for (DirectoryIterator it(full_path, nullptr, extensions); it != DirectoryIterator(); ++it)
    {
      std::string filename = it->d_name;
      std::string lowerCaseFilename = StringUtils::downcase(filename);
      // get the first letter
      auto name = StringUtils::upcase(filename);
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

  FileInfoVector getFileStartingWithPrefix(const char *folder, const char *prefix, const std::vector<std::string> &extensions)
  {
    auto bl = BusyLight();
    FileInfoVector files;
    if (!fileSystem->isMounted())
    {
      return files;
    }
    std::string full_path = std::string(fileSystem->mountPoint()) + folder;
    std::cout << "Listing directory: " << full_path << " for files starting with " << prefix << std::endl;
    for (const std::string &extension : extensions)
    {
      std::cout << "Extension: " << extension << std::endl;
    }

    for (DirectoryIterator it(full_path, prefix, extensions); it != DirectoryIterator(); ++it)
    {
      files.push_back(FileInfoPtr(new FileInfo(StringUtils::upcase(it->d_name), full_path + "/" + it->d_name)));
    }
    // sort the files - is this needed? Maybe they are already alphabetically sorted
    std::sort(files.begin(), files.end(), [](FileInfoPtr a, FileInfoPtr b)
              { return a->getTitle() < b->getTitle(); });
    return files;
  }
};

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
#include "FlashSPIFFS.h"
#include "FlashLittleFS.h"
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
  DirectoryIterator(const std::string &path, const char *prefix, std::vector<std::string> extensions, bool includeDirectories = false)
  : dirp(opendir(path.c_str())), 
    prefix(prefix), 
    extensions(extensions),
    includeDirectories(includeDirectories)
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
      : dirp(other.dirp), entry(other.entry), extensions(other.extensions), prefix(other.prefix), includeDirectories(other.includeDirectories)
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
  bool includeDirectories = false;

  bool isValidEntry()
  {
    if (!(entry->d_type == DT_REG || (includeDirectories && entry->d_type == DT_DIR)))
    {
      return false; // Not a file or directory
    }

    std::string filename = entry->d_name;
    if (filename[0] == '.')
    {
      return false; // Hidden file
    }

    std::string lowerCaseFilename = StringUtils::downcase(filename);
    // Note: We do not filter by first character here; bucketing for non-alphabetic is handled in getFileLetters.
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
      char firstChar = filename.empty() ? '\0' : filename[0];
      if (lowerCasePrefix == "#") {
        // Match files whose first character is NOT an alphabetic letter
        if (std::isalpha(static_cast<unsigned char>(firstChar))) {
          return false;
        }
      } else {
        if (lowerCaseFilename.length() < lowerCasePrefix.length() ||
            lowerCaseFilename.substr(0, lowerCasePrefix.length()) != lowerCasePrefix)
        {
          return false; // Prefix does not match
        }
      }
    }
    return true; // Entry is valid
  }
};

// File information class
class FileInfo
{
public:
  FileInfo(const std::string &title, const std::string &name, const std::string &path, bool isDirectory)
      : title(title), name(name), path(path), _isDirectory(isDirectory) {}
  std::string getTitle() const { return title; }
  std::string getName() const { return name; }
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
  bool isDirectory() { return _isDirectory; }
private:
  std::string title;
  std::string name;
  std::string path;
  bool _isDirectory;
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
  int getFileCount() const { return fileCount; }
private:
  std::string letter;
  int fileCount;
};

using FileLetterCountPtr = std::shared_ptr<FileLetterCount>;
using FileLetterCountVector = std::vector<FileLetterCountPtr>;

// Storage type enum for selecting flash or SD card
enum class StorageType {
  AUTO,  // Automatically select SD card if available, otherwise flash
  FLASH, // Force flash storage
  SD     // Force SD card storage
};

// Files class to list files in a directory
class IFiles
{
public:
  virtual bool isAvailable() = 0;
  virtual bool getSpace(uint64_t &total, uint64_t &used, StorageType storageType = StorageType::AUTO) = 0;
  virtual bool createDirectory(const char *folder) = 0;
  virtual FILE *open(const char *filename, const char *mode) = 0;
  virtual bool rename(const char *oldFilename, const char *newFilename) = 0;
  virtual bool remove(const char *filename) = 0;
  virtual FileLetterCountVector getFileLetters(const char *folder, const std::vector<std::string> &extensions, bool includeDirectories = false) = 0;
  virtual FileInfoVector getFileStartingWithPrefix(const char *folder, const char *prefix, const std::vector<std::string> &extensions, bool includeDirectories = false) = 0;
  virtual std::string getPath(const char *path) = 0;
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
    return fileSystem && fileSystem->isMounted();
  }

  bool getSpace(uint64_t &total, uint64_t &used, StorageType storageType = StorageType::AUTO)
  {
    if (!fileSystem || !fileSystem->isMounted())
    {
      return false;
    }
    return fileSystem->getSpace(total, used);
  }

  std::string getPath(const char *path)
  {
    std::string mount = fileSystem->mountPoint();
    std::string p = path ? path : "";

    // Remove any trailing slashes from the path
    while (!p.empty() && p.back() == '/') {
      p.pop_back();
    }

    // Remove trailing slash from mount unless it's just "/"
    if (mount.length() > 1 && mount.back() == '/') {
      mount.pop_back();
    }
    // Remove leading slash from path
    while (!p.empty() && p.front() == '/') {
      p.erase(0, 1);
    }
    // Special case: if mount is "/" (root), don't add extra slash
    if (mount == "/") {
      return mount + p;
    }
    return mount + "/" + p;
  }

  FILE *open(const char *filename, const char *mode)
  {
    auto bl = BusyLight();
    if (!fileSystem || !fileSystem->isMounted())
    {
      return nullptr;
    }
    std::string full_path = getPath(filename);
    return fopen(full_path.c_str(), mode);
  }
  bool rename(const char *oldFilename, const char *newFilename)
  {
    auto bl = BusyLight();
    if (!fileSystem || !fileSystem->isMounted())
    {
      return false;
    }
    std::string full_old_path = getPath(oldFilename);
    std::string full_new_path = getPath(newFilename);
    return ::rename(full_old_path.c_str(), full_new_path.c_str()) == 0;
  }
  bool remove(const char *filename)
  {
    auto bl = BusyLight();
    if (!fileSystem || !fileSystem->isMounted())
    {
      return false;
    }
    std::string full_path = getPath(filename);
    return ::remove(full_path.c_str()) == 0;
  }
  bool createDirectory(const char *folder)
  {
    auto bl = BusyLight();
    if (!fileSystem || !fileSystem->isMounted())
    {
      return -1;
    }
    std::string full_path = getPath(folder);
    // check to see if the folder exists
    struct stat st;
    if (stat(full_path.c_str(), &st) == -1)
    {
      return mkdir(full_path.c_str(), 0777) == -0;
    }
    return false;
  }

  FileLetterCountVector getFileLetters(const char *folder, const std::vector<std::string> &extensions, bool includeDirectories = false)
  {
    auto bl = BusyLight();
    FileLetterCountVector fileLetters;
    if (!fileSystem || !fileSystem->isMounted())
    {
      return fileLetters;
    }
    std::string full_path = getPath(folder);
    std::cout << "Listing directory: " << full_path << std::endl;

    std::map<std::string, int> fileCountByLetter;

    for (DirectoryIterator it(full_path, nullptr, extensions); it != DirectoryIterator(); ++it)
    {
      std::string filename = it->d_name;
      std::string lowerCaseFilename = StringUtils::downcase(filename);
      // get the first letter
      auto name = StringUtils::upcase(filename);
      char firstChar = name.empty() ? '\0' : name[0];
      std::string letter;
      if (std::isalpha(static_cast<unsigned char>(firstChar))) {
        letter = name.substr(0, 1);
      } else {
        letter = "#";
      }
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

  FileInfoVector getFileStartingWithPrefix(const char *folder, const char *prefix, const std::vector<std::string> &extensions, bool includeDirectories = false)
  {
    auto bl = BusyLight();
    FileInfoVector files;
    if (!fileSystem || !fileSystem->isMounted())
    {
      return files;
    }
    std::string full_path = getPath(folder);
    std::cout << "Listing directory: " << full_path << " for files starting with " << prefix << std::endl;
    for (const std::string &extension : extensions)
    {
      std::cout << "Extension: " << extension << std::endl;
    }

    for (DirectoryIterator it(full_path, prefix, extensions, includeDirectories); it != DirectoryIterator(); ++it)
    {
      if (it->d_type == DT_REG) {
        // do we need a slash at the end of the path?
        if (full_path[full_path.length() - 1] != '/')
        {
          full_path += "/";
        }
        std::string fullFilePath = full_path + it->d_name;
      }
      files.push_back(FileInfoPtr(new FileInfo(StringUtils::upcase(it->d_name), it->d_name, full_path + it->d_name, it->d_type == DT_DIR)));
    }
    // sort the files - is this needed? Maybe they are already alphabetically sorted
    std::sort(files.begin(), files.end(), [](FileInfoPtr a, FileInfoPtr b)
              { return a->getTitle() < b->getTitle(); });
    return files;
  }
};

class UnifiedStorage : public IFiles {
private:
  IFiles *flashFiles;
  IFiles *sdFiles;

public:
  UnifiedStorage(IFiles* flash, IFiles* sd) 
    : flashFiles(flash), sdFiles(sd) {}

  bool isAvailable() override {
    return (flashFiles->isAvailable()) || (sdFiles->isAvailable());
  }

  bool getSpace(uint64_t &total, uint64_t &used, StorageType storageType = StorageType::AUTO) override {
    if (storageType == StorageType::FLASH) {
      // Force flash storage
      return flashFiles->getSpace(total, used);
    } else if (storageType == StorageType::SD) {
      // Force SD card storage
      if (sdFiles->isAvailable()) {
        return sdFiles->getSpace(total, used);
      }
      // SD card not available
      total = 0;
      used = 0;
      return false;
    } else {
      // Auto mode - use SD card if available, otherwise use flash
      if (sdFiles->isAvailable()) {
        return sdFiles->getSpace(total, used);
      }
      return flashFiles->getSpace(total, used);
    }
  }

  std::string getPath(const char *path) override {
    if (sdFiles->isAvailable()) {
      return sdFiles->getPath(path);
    }
    return flashFiles->getPath(path);
  }

  bool createDirectory(const char *folder) override {
    if (sdFiles->isAvailable()) {
      return sdFiles->createDirectory(folder);
    }
    // fallback to flash
    if (flashFiles->isAvailable()) {
      return flashFiles->createDirectory(folder);
    }
    return -1;
  }

  FILE *open(const char *filename, const char *mode) override {
    if (sdFiles->isAvailable()) {
      return sdFiles->open(filename, mode);
    }
    return flashFiles->open(filename, mode);
  }

  bool rename(const char *oldFilename, const char *newFilename) override {
    if (sdFiles->isAvailable()) {
      return sdFiles->rename(oldFilename, newFilename);
    }
    // fallback to flash
    if (flashFiles->isAvailable()) {
      return flashFiles->rename(oldFilename, newFilename);
    }
    return false;
  }

  bool remove(const char *filename) override {
    if (sdFiles->isAvailable()) {
      return sdFiles->remove(filename);
    }
    // fallback to flash
    if (flashFiles->isAvailable()) {
      return flashFiles->remove(filename);
    }
    return false;
  }

  FileLetterCountVector getFileLetters(const char *folder, const std::vector<std::string> &extensions, bool includeDirectories = false) override {
    std::map<std::string, int> letterCountMap;
    
    // Get files from both sources
    if (flashFiles->isAvailable()) {
      auto flashLetters = flashFiles->getFileLetters(folder, extensions, includeDirectories);
      for (const auto& letterCount : flashLetters) {
        letterCountMap[letterCount->getLetter()] = letterCount->getFileCount();
      }
    }
    
    if (sdFiles->isAvailable()) {
      auto sdLetters = sdFiles->getFileLetters(folder, extensions, includeDirectories);
      for (const auto& letterCount : sdLetters) {
        auto it = letterCountMap.find(letterCount->getLetter());
        if (it != letterCountMap.end()) {
          letterCountMap[letterCount->getLetter()] = it->second + letterCount->getFileCount();
        } else {
          letterCountMap[letterCount->getLetter()] = letterCount->getFileCount();
        }
      }
    }

    // Convert map back to vector
    FileLetterCountVector combined;
    for (const auto& pair : letterCountMap) {
      combined.push_back(std::make_shared<FileLetterCount>(pair.first, pair.second));
    }
    
    // Sort alphabetically
    std::sort(combined.begin(), combined.end(), 
      [](const FileLetterCountPtr& a, const FileLetterCountPtr& b) {
        return a->getLetter() < b->getLetter();
      });
    
    return combined;
  }

  FileInfoVector getFileStartingWithPrefix(const char *folder, const char *prefix, const std::vector<std::string> &extensions, bool includeDirectories = false) override {
    FileInfoVector combined;
    std::map<std::string, FileInfoPtr> uniqueFiles;

    // Get files from both sources
    if (flashFiles->isAvailable()) {
      auto flashFiles = this->flashFiles->getFileStartingWithPrefix(folder, prefix, extensions, includeDirectories);
      for (const auto& file : flashFiles) {
        uniqueFiles[file->getTitle()] = file;
      }
    }

    if (sdFiles->isAvailable()) {
      auto sdFiles = this->sdFiles->getFileStartingWithPrefix(folder, prefix, extensions, includeDirectories);
      for (const auto& file : sdFiles) {
        // Only add if not already present from flash
        if (uniqueFiles.find(file->getTitle()) == uniqueFiles.end()) {
          uniqueFiles[file->getTitle()] = file;
        }
      }
    }

    // Convert map back to vector
    for (const auto& pair : uniqueFiles) {
      combined.push_back(pair.second);
    }

    // Sort alphabetically
    std::sort(combined.begin(), combined.end(),
      [](const FileInfoPtr& a, const FileInfoPtr& b) {
        return a->getTitle() < b->getTitle();
      });

    return combined;
  }
};

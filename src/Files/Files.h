#pragma once

#include <vector>
#include <string>

class FileInfo {
  public:
    std::string name;
    std::string path;
    FileInfo(std::string name, std::string path) : name(name), path(path) {}
};

class SDCard;
class Flash;
class Files {
  private:
    SDCard *sdCard;
    Flash *flash;
  public:
    Files();
    std::vector<FileInfo *> listFilePaths(const char *folder, const char *extension=NULL);
};

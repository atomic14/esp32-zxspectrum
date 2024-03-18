#pragma once

#include <vector>
#include <string>

#define MOUNT_POINT "/fs"

class Files {
  public:
    std::vector<std::string> listFiles(const char *folder, const char *extension=NULL);
};

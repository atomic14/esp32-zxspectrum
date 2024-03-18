#include <Arduino.h>
#include "Files.h"
#include <stdio.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include <dirent.h>


std::vector<std::string> Files::listFiles(const char *folder, const char *extension)
{
  std::vector<std::string> files;
  char full_path[100];
  sprintf(full_path, "/%s%s", MOUNT_POINT, folder);
  Serial.printf("Listing directory: %s\n", full_path);

  // open the directory
  DIR *dir = opendir(full_path);
  if (!dir)
  {
    Serial.println("Failed to open directory");
    return files;
  }
  // list all the files in the directory that end with the extension
  struct dirent *ent;
  while ((ent = readdir(dir)) != NULL)
  {
    std::string filename = "/" + std::string(ent->d_name);
    bool isFile = ent->d_type == DT_REG;
    bool isVisible = filename[0] != '.';
    bool isMatchingExtension = extension == NULL || filename.find(extension) == filename.length() - strlen(extension);
    if (isFile && isVisible && isMatchingExtension)
    {
      files.push_back(MOUNT_POINT + filename);
    }
  }
  // sort the files alphabetically
  std::sort(files.begin(), files.end());
  return files;
}

#include <Arduino.h>
#include <stdio.h>
#include <filesystem>
#include <sys/unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include "Files.h"
#include "SDCard.h"
#include "Flash.h"

static const char *MOUNT_POINT="/fs";

Files::Files() {
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

std::vector<FileInfo *> Files::listFilePaths(const char *folder, const char *extension)
{
  std::vector<FileInfo *> files;
  char full_path[100];
  sprintf(full_path, "%s%s", MOUNT_POINT, folder);
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
    std::string filename = std::string(ent->d_name);
    bool isFile = ent->d_type == DT_REG;
    bool isVisible = filename[0] != '.';
    bool isMatchingExtension = extension == NULL || filename.find(extension) == filename.length() - strlen(extension);
    if (isFile && isVisible && isMatchingExtension)
    {
      files.push_back(new FileInfo(filename, std::string(folder) + "/" + filename));
    }
  }
  // sort the files alphabetically
  std::sort(files.begin(), files.end());
  return files;
}

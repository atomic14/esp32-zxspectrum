#include "./Files.h"
#include <ArduinoJson.h>
#include <cstdio>
#include <sstream>
#include <string>
#include "ISettings.h"

class Settings: public ISettings {
  private:
    // called automatically when settings are updated
    bool save() {
      FILE *file = m_files->open("settings.json", "w");
      if (file) {
        std::stringstream ss;
        serializeJson(doc, ss);
        std::string jsonStr = ss.str();
        Serial.printf("Saving settings: %s\n", jsonStr.c_str());
        fwrite(jsonStr.c_str(), 1, jsonStr.size(), file);
        fclose(file);
        return true;
      } else {
        Serial.println("Failed to open settings file");
      }
      return false;
    }
  public:
    Settings(IFiles *files) : m_files(files) {
      bool loaded = false;
      FILE *file = m_files->open("settings.json", "r");
      if (file) {
        fseek(file, 0, SEEK_END);
        long fileSize = ftell(file);
        fseek(file, 0, SEEK_SET);
        if (fileSize > 0) {
          std::string jsonStr(fileSize, '\0');
          fread(&jsonStr[0], 1, fileSize, file);
          std::stringstream ss(jsonStr);
          DeserializationError error = deserializeJson(doc, ss);
          loaded = error.code() == DeserializationError::Ok;
        }
        fclose(file);
      }
      // populate the default values
      if (!loaded) {
        Serial.println("No settings found, creating default");
        doc["volume"] = 10;
        save();
      }
    }
    int getVolume() {
      return doc["volume"];
    }
    void setVolume(int volume) {
      doc["volume"] = volume;
      save();
    }
  private:
    IFiles *m_files;
    JsonDocument doc;
};
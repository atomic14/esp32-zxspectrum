#pragma once

#include <string>

class FlashLittleFS
{
private:
  bool _isMounted;
  std::string m_mountPoint;
public:
  static constexpr const char* DEFAULT_MOUNT_POINT = "/littlefs";
  FlashLittleFS(const char *mountPoint);
  ~FlashLittleFS();
  bool isMounted() {
    return _isMounted;
  }
  const char *mountPoint() {
    return m_mountPoint.c_str();
  }
};
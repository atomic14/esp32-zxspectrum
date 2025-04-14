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
  bool getSpace(uint64_t &total, uint64_t &used);
  const char *mountPoint() {
    return m_mountPoint.c_str();
  }
};
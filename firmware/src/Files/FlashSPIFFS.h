#pragma once

#include <string>

class FlashSPIFFS
{
private:
  bool _isMounted;
  std::string m_mountPoint;
public:
  static constexpr const char* DEFAULT_MOUNT_POINT = "/flash";
  FlashSPIFFS(const char *mountPoint);
  ~FlashSPIFFS();
  bool isMounted() {
    return _isMounted;
  }
  const char *mountPoint() {
    return m_mountPoint.c_str();
  }
};
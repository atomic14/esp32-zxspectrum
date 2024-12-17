#pragma once

class Flash
{
private:
  bool _isMounted;
  std::string m_mountPoint;
public:
  static constexpr const char* DEFAULT_MOUNT_POINT = "/flash";
  Flash(const char *mountPoint);
  ~Flash();
  bool isMounted() {
    return _isMounted;
  }
  const char *mountPoint() {
    return m_mountPoint.c_str();
  }
};
#pragma once

class Flash
{
private:
  bool _isMounted;
  const char *_mountPoint;
public:
  Flash(const char *mountPoint);
  ~Flash();
  bool isMounted() {
    return _isMounted;
  }
  const char *mountPoint() {
    return _mountPoint;
  }
};
#pragma once

class ISettings {
  public:
    virtual int getVolume() = 0;
    virtual void setVolume(int volume) = 0;
};

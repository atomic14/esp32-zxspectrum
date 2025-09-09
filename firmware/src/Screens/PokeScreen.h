#pragma once

#include "MessageScreen.h"
#include "NavigationStack.h"
#include "Screen.h"
#include "../TFT/TFTDisplay.h"
#include "fonts/GillSans_25_vlw.h"
#include "fonts/GillSans_15_vlw.h"
#include "../Emulator/spectrum.h"
#include "../Emulator/snaps.h"


class IFiles;

class PokeScreen : public Screen
{
private:
  ZXSpectrum *machine = nullptr;
  std::string s_addr = "0x5800";
  std::string s_data = "";
  byte active_control = 0;
public:
  PokeScreen(
      Display &tft,
      HDMIDisplay *hdmiDisplay,
      AudioOutput *audioOutput,
      ZXSpectrum *machine) : machine(machine), Screen(tft, hdmiDisplay, audioOutput, nullptr)
  {
  }

  void didAppear()
  {
    updateDisplay();
  }

  void pressKey(SpecKeys key);
  void updateDisplay();

private:
  std::vector<std::string> split(const std::string &s, char delim);
  
  // Parse a string as a number with the specified base (10 if not specified).
  // If the string starts with 0x the base is ignored and the 
  // string is parsed as hex
  int16_t get_num(const std::string &s, int base);

  // Parse a string of bytes delimited by single spaces
  std::vector<byte> get_bytes(const std::string &s, int base);

  std::string getControlString();
  void setControlString(std::string str);
};

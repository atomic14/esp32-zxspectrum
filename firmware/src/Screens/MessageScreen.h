#pragma once

#include "NavigationStack.h"
#include "Screen.h"
#include "../TFT/TFTDisplay.h"
#include "fonts/GillSans_25_vlw.h"
#include "fonts/GillSans_15_vlw.h"

class IFiles;

class MessageScreen : public Screen
{
private:
  std::string m_title;
  std::vector<std::string> m_messages;
public:
  MessageScreen(
      std::string title,
      std::vector<std::string> messages,
      Display &tft,
      HDMIDisplay *hdmiDisplay,
      AudioOutput *audioOutput) : m_title{title}, m_messages{messages}, Screen(tft, hdmiDisplay, audioOutput, nullptr)
  {
  }

  void didAppear()
  {
    updateDisplay();
  }

  void pressKey(SpecKeys key)
  {
    m_navigationStack->pop();
  }

  void updateDisplay();
};

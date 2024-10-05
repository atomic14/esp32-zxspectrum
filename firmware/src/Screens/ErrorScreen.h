#pragma once

#include "Screen.h"
#include "../TFT/TFTDisplay.h"
#include "font.h"

class TFTDisplay;

class ErrorScreen : public Screen
{
private:
  // back callback
  using BackCallback = std::function<void()>;
  BackCallback m_backCallback;
  std::vector<std::string> m_messages;

public:
  ErrorScreen(
      TFTDisplay &tft,
      AudioOutput *audioOutput,
      std::vector<std::string> messages,
      BackCallback backCallback) : Screen(tft, audioOutput), m_messages(messages), m_backCallback(backCallback)
  {
    m_tft.loadFont(GillSans_30_vlw);
  }

  void didAppear()
  {
    updateDisplay();
  }
  
  void updatekey(SpecKeys key, uint8_t state)
  {
    if (state == 1)
    {
        m_backCallback();
    }
  }

  void updateDisplay()
  {
    m_tft.startWrite();
    m_tft.fillScreen(TFT_RED);
    m_tft.setTextColor(TFT_WHITE, TFT_RED);
    for(int i = 0; i < m_messages.size(); i++)
    {
      m_tft.drawString(m_messages[i].c_str(), 20, 50 + (i * 40));
    }
    m_tft.endWrite();
  }
};

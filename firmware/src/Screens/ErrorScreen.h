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
      BackCallback backCallback) : Screen(tft, audioOutput), m_backCallback(backCallback)
  {
    m_tft.loadFont(GillSans_30_vlw);
  }

  void setMessages(std::vector<std::string> messages)
  {
    m_messages = messages;
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
    int startY = (m_tft.height() - 40 * m_messages.size())/2;
    for(int i = 0; i < m_messages.size(); i++)
    {
      int width = m_tft.measureString(m_messages[i].c_str()).x;
      int startX = (m_tft.width() - width) / 2;
      m_tft.drawString(m_messages[i].c_str(), startX, startY + (i * 40));
    }
    m_tft.endWrite();
  }
};

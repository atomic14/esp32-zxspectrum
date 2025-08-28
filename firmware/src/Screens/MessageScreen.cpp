#include "MessageScreen.h"

void MessageScreen::updateDisplay()
{
    m_tft.startWrite();
    m_tft.fillRect(0, 0, m_tft.width(), m_tft.height(), TFT_BLACK);
    m_tft.drawRect(0, 0, m_tft.width(), m_tft.height(), TFT_WHITE);

    m_tft.loadFont(GillSans_25_vlw);
    m_tft.drawCenterString(m_title.c_str(), 5);

    m_tft.loadFont(GillSans_15_vlw);
    int16_t y = 30;
    for (int i=0; i<m_messages.size(); i++){
        m_tft.drawString(m_messages[i].c_str(), 5, y);
        y += 20;
    }

    m_tft.endWrite();
}
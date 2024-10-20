#pragma once

#include <vector>
#include <string>
#include "Screen.h"
#include "../TFT/TFTDisplay.h"
#include "../Emulator/spectrum.h"
#include "fonts/GillSans_30_vlw.h"
#include "fonts/GillSans_15_vlw.h"
#include "images/rainbow_image.h"

class TFTDisplay;
class ScrollingList;

bool starts_with(const std::string& str, const std::string& prefix) {
    return str.size() >= prefix.size() && str.compare(0, prefix.size(), prefix) == 0;
}

template <class ItemT>
class PickerScreen : public Screen
{
private:
  std::vector<ItemT> m_items;
  std::string searchPrefix;
  std::string title;
  int lastSearchPrefix = 0;
  int m_selectedItem = 0;
  int m_lastPageDrawn = -1;
public:
  PickerScreen(
      std::string title,
      TFTDisplay &tft,
      AudioOutput *audioOutput
  ) : title(title), Screen(tft, audioOutput)
  {
  }

  void setItems(std::vector<ItemT> items)
  {
    m_items = items;
    m_selectedItem = 0;
    m_lastPageDrawn = -1;
    updateDisplay();
  }


  void didAppear()
  {
    m_lastPageDrawn=-1;
    updateDisplay();
  }

  virtual void onBack() {
    m_navigationStack->pop();
  }
  virtual void onItemSelect(ItemT item, int index) = 0;

  void pressKey(SpecKeys key)
  {
    bool isHandled = false;
    switch (key)
    {
    case JOYK_UP:
    case SPECKEY_7:
      if (m_selectedItem > 0)
      {
        playKeyClick();
        m_selectedItem--;
        updateDisplay();
      }
      isHandled = true;
      break;
    case JOYK_DOWN:
    case SPECKEY_6:
      if (m_selectedItem < m_items.size() - 1)
      {
        playKeyClick();
        m_selectedItem++;
        updateDisplay();
      }
      isHandled = true;
      break;
    case JOYK_LEFT:
    case SPECKEY_5:
    {
      playKeyClick();
      isHandled = true;
      onBack();
      break;
    }
    case JOYK_FIRE:
    case SPECKEY_ENTER:
      playKeyClick();
      onItemSelect(m_items[m_selectedItem], m_selectedItem);
      break;
    }
    if (!isHandled)
    {
      // does the speckey map onto a letter - look in the mapping table
      if (specKeyToLetter.find(key) != specKeyToLetter.end())
      {
        playKeyClick();
        char letter = specKeyToLetter.at(key);
        if (millis() - lastSearchPrefix > 500)
        {
          searchPrefix = "";
        }
        searchPrefix += letter;
        lastSearchPrefix = millis();
        for (int i = 0; i < m_items.size(); i++)
        {
          if (starts_with(m_items[i]->getTitle(), searchPrefix))
          {
            m_selectedItem = i;
            updateDisplay();
            break;
          }
        }
      }
    }
  }

  void updateDisplay()
  {
    m_tft.startWrite();
    int linesPerPage = (m_tft.height() - 10 - 15)/30;
    int page = m_selectedItem / linesPerPage;
    if (page != m_lastPageDrawn)
    {
      m_tft.fillScreen(TFT_BLACK);
      m_lastPageDrawn = page;
    }
    m_tft.loadFont(GillSans_15_vlw);
    m_tft.setTextColor(TFT_WHITE, TFT_BLACK);
    m_tft.drawString((title + " - 5: Back, 6: Down, 7: Up, ENTER: Pick").c_str(), 0, 0);
    m_tft.drawFastHLine(0, 15, m_tft.width() - 1, TFT_WHITE);
    m_tft.loadFont(GillSans_30_vlw);
    for (int i = 0; i < linesPerPage; i++)
    {
      int itemIndex = page * linesPerPage + i;
      if (itemIndex >= m_items.size())
      {
        break;
      }
      m_tft.setTextColor(itemIndex == m_selectedItem ? TFT_GREEN : TFT_WHITE, TFT_BLACK);
      m_tft.drawString(m_items[itemIndex]->getTitle().c_str(), 20, 10 + 15 + i * 30);
    }
    // draw the spectrum flash
    m_tft.setWindow(TFT_HEIGHT - rainbowImageWidth, TFT_WIDTH - rainbowImageHeight, TFT_HEIGHT - 1, TFT_WIDTH - 1);
    m_tft.pushPixels((uint16_t *) rainbowImageData, rainbowImageWidth * rainbowImageHeight);
    m_tft.endWrite();
  }
};

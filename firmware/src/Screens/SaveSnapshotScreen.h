#pragma once

#include "Screen.h"
#include "../TFT/TFTDisplay.h"
#include "fonts/GillSans_30_vlw.h"
#include "fonts/GillSans_15_vlw.h"
#include "../Emulator/spectrum.h"
#include "../Emulator/snaps.h"

class TFTDisplay;

class SaveSnapshotScreen : public Screen
{
private:
  ZXSpectrum *machine = nullptr;
  std::string filename = "";
public:
  SaveSnapshotScreen(
      TFTDisplay &tft,
      AudioOutput *audioOutput,
      ZXSpectrum *machine) : machine(machine), Screen(tft, audioOutput)
  {
  }

  void didAppear()
  {
    updateDisplay();
  }

  void pressKey(SpecKeys key) 
  {
    switch (key)
    {
    case JOYK_FIRE:
    case SPECKEY_ENTER:
    if (filename.length() > 0) {
        auto bl = BusyLight();
        drawBusy();
        saveZ80(machine, ("/fs/snapshots/" + filename + ".Z80").c_str());
        playSuccessBeep();
        vTaskDelay(500 / portTICK_PERIOD_MS);
        m_navigationStack->pop();
        return;
    }
    case SPECKEY_DEL:
      if (filename.length() > 0)
      {
        Serial.println("Delete last character");
        filename = filename.substr(0, filename.length() - 1);
        updateDisplay();
        playKeyClick();
        return;
      }
      else
      {
        playErrorBeep();
      }
      break;
    case SPECKEY_BREAK:
      Serial.println("Cancel");
      m_navigationStack->pop();
      return;
    default:
      // does the speckey map onto a letter - look in the mapping table
      if (specKeyToLetter.find(key) != specKeyToLetter.end())
      {
        char letter = specKeyToLetter.at(key);
        // only allow alphnum and not "."
        if (isalpha(letter) || isdigit(letter))
        {
          // maximum length of 8
          if (filename.length() < 8)
          {
            // make upper case
            letter = toupper(letter);
            filename += letter;
            playKeyClick();
            updateDisplay();
            return;
          }
          else
          {
            playErrorBeep();
          }
        }
      }
    }
  };
  

  void updateDisplay()
  {
    const int yMargin = 100;
    const int xMargin = 76;
    m_tft.loadFont(GillSans_30_vlw);
    m_tft.startWrite();
    m_tft.fillRect(xMargin/2, yMargin/2, m_tft.width() - xMargin, m_tft.height() - yMargin, TFT_BLACK);
    m_tft.drawRect(xMargin/2, yMargin/2, m_tft.width() - xMargin, m_tft.height() - yMargin, TFT_WHITE);
    m_tft.setTextColor(TFT_WHITE, TFT_BLACK);
    auto size = m_tft.measureString("Enter Filename");
    m_tft.drawString("Enter Filename", (m_tft.width() - size.x)/2, yMargin/2 + 15);

    int centerX = m_tft.width() / 2;
    int centerY = m_tft.height() / 2;

    // filename textbox
    m_tft.fillRect(centerX - 4*25, centerY-20, 8*25, 40, 0x2020);
    m_tft.drawRect(centerX - 4*25, centerY-20, 8*25, 40, TFT_WHITE);

    m_tft.setTextColor(TFT_WHITE, 0x2020);
    auto textSize = m_tft.measureString(filename.c_str());
    m_tft.drawString(filename.c_str(), centerX - textSize.x/2, centerY - textSize.y/2);

    // draw a cursor on the end of the filename (let's just use a white rectangle)
    m_tft.fillRect(centerX + textSize.x/2, centerY-15, 3, 30, TFT_WHITE);

    m_tft.loadFont(GillSans_15_vlw);
    auto instructionsSize = m_tft.measureString("Press ENTER to save, BREAK to exit");
    m_tft.drawString("Press ENTER to save, BREAK to exit", centerX - instructionsSize.x/2, centerY + 40);

    m_tft.endWrite();
  }
};

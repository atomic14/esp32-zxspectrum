#pragma once

#include "Screen.h"
#include "../TFT/Display.h"
#include "fonts/GillSans_25_vlw.h"
#include "fonts/GillSans_15_vlw.h"
#include "NavigationStack.h"
#include "version_info.h"
#include "images/rainbow_image.h"
#include "../Files/Files.h"
#include "TestScreen.h"

class AboutScreen : public Screen
{
private:
  // Helper function to format storage size in KB, MB, or GB
  std::string formatStorage(uint64_t sizeInBytes) {
    char buffer[32];
    if (sizeInBytes < 1000) {
      sprintf(buffer, "%d B", (int)sizeInBytes);
    } else if (sizeInBytes < 1000 * 1000) {
      sprintf(buffer, "%.1f KB", sizeInBytes / 1000.0f);
    } else if (sizeInBytes < 1000 * 1000 * 1000) {
      sprintf(buffer, "%.1f MB", sizeInBytes / (1000.0f * 1000.0f));
    } else {
      sprintf(buffer, "%.2f GB", sizeInBytes / (1000.0f * 1000.0f * 1000.0f));
    }
    return std::string(buffer);
  }

  void updateDisplay()
  {
    m_tft.loadFont(GillSans_25_vlw);
    m_tft.startWrite();
    m_tft.fillScreen(TFT_BLACK);
    
    // Title
    m_tft.setTextColor(TFT_WHITE, TFT_BLACK);
    int titleY = 20;
    std::string title = "ESP32 ZX Spectrum";
    int titleWidth = m_tft.measureString(title.c_str()).x;
    int titleX = (m_tft.width() - titleWidth) / 2;
    m_tft.drawString(title.c_str(), titleX, titleY);
    
    // Firmware version
    m_tft.loadFont(GillSans_15_vlw);
    m_tft.setTextColor(TFT_WHITE, TFT_BLACK);
    int textY = titleY + 40;
    std::string firmwareVersion = "Firmware: ";
    firmwareVersion += FIRMWARE_VERSION_STRING;
    int textWidth = m_tft.measureString(firmwareVersion.c_str()).x;
    int textX = (m_tft.width() - textWidth) / 2;
    m_tft.drawString(firmwareVersion.c_str(), textX, textY);
    
    // Hardware version
    textY += 25;
    std::string hardwareVersion = "Hardware: ";
    hardwareVersion += HARDWARE_VERSION_STRING;
    textWidth = m_tft.measureString(hardwareVersion.c_str()).x;
    textX = (m_tft.width() - textWidth) / 2;
    m_tft.drawString(hardwareVersion.c_str(), textX, textY);
    
    // Flash storage info
    textY += 25;
    uint64_t flashTotal = 0, flashUsed = 0;
    std::string flashInfo = "Flash: ";
    
    if (m_files->getSpace(flashTotal, flashUsed, StorageType::FLASH)) {
      flashInfo += formatStorage(flashUsed) + " used / " + formatStorage(flashTotal);
    } else {
      flashInfo += "Not available";
    }
    
    textWidth = m_tft.measureString(flashInfo.c_str()).x;
    textX = (m_tft.width() - textWidth) / 2;
    m_tft.drawString(flashInfo.c_str(), textX, textY);
    
    // SD Card info
    textY += 25;
    uint64_t sdTotal = 0, sdUsed = 0;
    std::string sdInfo = "SD Card: ";
    
    if (m_files->getSpace(sdTotal, sdUsed, StorageType::SD)) {
      if (sdUsed == 0) {
        // For SD cards, we might not have used space information
        sdInfo += formatStorage(sdTotal) + " total";
      } else {
        sdInfo += formatStorage(sdUsed) + " used / " + formatStorage(sdTotal);
      }
    } else {
      sdInfo += "Not mounted";
    }
    
    textWidth = m_tft.measureString(sdInfo.c_str()).x;
    textX = (m_tft.width() - textWidth) / 2;
    m_tft.drawString(sdInfo.c_str(), textX, textY);
    
    // Footer instructions
    int footerY = m_tft.height() - 40;
    std::string footer = "Press any key to return";
    int footerWidth = m_tft.measureString(footer.c_str()).x;
    int footerX = (m_tft.width() - footerWidth) / 2;
    m_tft.drawString(footer.c_str(), footerX, footerY);
    
    // Draw the spectrum flash
    m_tft.setWindow(m_tft.width() - rainbowImageWidth, m_tft.height() - rainbowImageHeight, m_tft.width() - 1, m_tft.height() - 1);
    m_tft.pushPixels((uint16_t *) rainbowImageData, rainbowImageWidth * rainbowImageHeight);

    m_tft.endWrite();
  }

public:
  AboutScreen(
      Display &tft,
      HDMIDisplay *hdmiDisplay,
      AudioOutput *audioOutput,
      IFiles *files) : Screen(tft, hdmiDisplay, audioOutput, files)
  {
  }

  void didAppear() override
  {
    updateDisplay();
  }
  
  void pressKey(SpecKeys key) override
  {
    // If the 't' key is pressed, go to the test screen
    if (key == SPECKEY_T) {
      TestScreen *testScreen = new TestScreen(m_tft, m_hdmiDisplay, m_audioOutput, m_files);
      m_navigationStack->push(testScreen);
    } else {
      m_navigationStack->pop();
    }
  }
};
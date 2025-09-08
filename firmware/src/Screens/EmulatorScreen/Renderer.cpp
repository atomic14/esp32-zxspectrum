#include "Renderer.h"
#include "../../TFT/HDMIDisplay.h"
#include "../../Emulator/spectrum.h"
#include "../fonts/GillSans_15_vlw.h"
#include "../../AudioOutput/AudioOutput.h"

static const int VOLUME_BAR_HEIGHT = 45;
static const int MENU_BAR_HEIGHT = 20;

void displayTask(void *pvParameters) {
  Renderer *renderer = (Renderer *)pvParameters;
  while (1)
  {
    if (renderer->isRunning)
    {
      if (xSemaphoreTake(renderer->m_displaySemaphore, portMAX_DELAY))
      {
        renderer->drawScreen();
      }
    }
    else 
    {
      vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    // if (renderer->isRunning && digitalRead(0) == LOW)
    // {
    //   renderer->pause();
    //   renderer->showSaveSnapshotScreen();
    // }
  }
}

void Renderer::drawBorder(int startPos, int endPos, int offset, int length, int drawWidth, int drawHeight, bool isSideBorders)
{
  for (int borderPos = startPos; borderPos < endPos;)
  {
    uint8_t borderColor = currentBorderColors[borderPos + offset];
    if (drawnBorderColors[borderPos + offset] != borderColor || firstDraw)
    {
      // Find consecutive lines with the same color
      int rangeStart = borderPos;
      while (borderPos < endPos && ((currentBorderColors[borderPos + offset] == borderColor &&
              drawnBorderColors[borderPos + offset] != borderColor) || firstDraw))
      {
        drawnBorderColors[borderPos + offset] = borderColor;
        borderPos++;
      }
      int rangeLength = borderPos - rangeStart;

      uint16_t tftColor = specpal565[borderColor];
      tftColor = (tftColor >> 8) | (tftColor << 8);

      if (isSideBorders)
      {
        // Draw left and right borders
        m_tft.fillRect(0, rangeStart, length, rangeLength, tftColor);                  // Left side
        m_tft.fillRect(drawWidth - length, rangeStart, length, rangeLength, tftColor); // Right side
      }
      else
      {
        // Draw top or bottom borders
        m_tft.fillRect(0, rangeStart, drawWidth, rangeLength, tftColor);
      }
    }
    else
    {
      borderPos++;
    }
  }
}

void Renderer::drawScreen()
{
  if (m_HDMIDisplay) {
    m_tft.dmaWait();
    m_HDMIDisplay->sendSpectrum(currentScreenBuffer, currentBorderColors);
  }
  drawSpectrumScreen();
  m_tft.startWrite();
  m_tft.dmaWait();
  if (isShowingMenu) {
    drawMenu();
  } else if (isShowingTimeTravel) {
    drawTimeTravel();
  }
  m_tft.endWrite();
}

void Renderer::drawSpectrumScreen() {
  if (isLoading)
  {
    int position = loadProgress * screenWidth / 100;
    m_tft.fillRect(position, 0, screenWidth - position, 8, TFT_BLACK);
    m_tft.fillRect(0, 0, position, 8, TFT_GREEN);
  }
  int borderOffset = 36;

  int borderHeightSkip = (isShowingMenu | isShowingTimeTravel) ? MENU_BAR_HEIGHT : isLoading ? 8 : 0;

  // Draw the top border
  drawBorder(borderHeightSkip, borderHeight, borderOffset, screenWidth, screenWidth, borderHeight, false);

  // Draw the bottom border - unless we are showing the menu which includes the bottom volume bar
  if (!isShowingMenu) {
    drawBorder(screenHeight - borderHeight, screenHeight, borderOffset, screenWidth, screenWidth, borderHeight, false);
  }

  int bottomBorderSkip = isShowingMenu ? VOLUME_BAR_HEIGHT : 0;

  // Draw the left and right borders
  drawBorder(borderHeightSkip, screenHeight - borderHeight - bottomBorderSkip, borderOffset, borderWidth, screenWidth, borderHeight, true);
  // do the pixels
  uint8_t *attrBase = currentScreenBuffer + 0x1800;
  uint8_t *pixelBase = currentScreenBuffer;
  uint8_t *attrBaseCopy = screenBuffer + 0x1800;
  uint8_t *pixelBaseCopy = screenBuffer;
  for (int attrY = 0; attrY < 192 / 8; attrY++)
  {
    bool dirty = false;
    for (int attrX = 0; attrX < 256 / 8; attrX++)
    {
      // read the value of the attribute
      uint8_t attr = *(attrBase + 32 * attrY + attrX);
      uint8_t inkColor = attr & B00000111;
      uint8_t paperColor = (attr & B00111000) >> 3;
      // check for changes in the attribute
      if ((attr & B10000000) != 0 && flashTimer < 16)
      {
        // we are flashing we need to swap the ink and paper colors
        uint8_t temp = inkColor;
        inkColor = paperColor;
        paperColor = temp;
        // update the attribute with the new colors - this makes our dirty check work
        attr = (attr & B11000000) | (inkColor & B00000111) | ((paperColor << 3) & B00111000);
      }
      if (attr != *(attrBaseCopy + 32 * attrY + attrX))
      {
        dirty = true;
        *(attrBaseCopy + 32 * attrY + attrX) = attr;
      }
      if ((attr & B01000000) != 0)
      {
        inkColor = inkColor + 8;
        paperColor = paperColor + 8;
      }
      uint16_t tftInkColor = specpal565[inkColor];
      uint16_t tftPaperColor = specpal565[paperColor];
      const uint32_t u32Lookup[4] = {
        tftPaperColor | (tftPaperColor << 16), // 00
        tftInkColor | (tftPaperColor << 16), // 01
        tftPaperColor | (tftInkColor << 16), // 10
        tftInkColor | (tftInkColor << 16) // 11
      };
      for (int y = 0; y < 8; y++)
      {
        // read the value of the pixels
        int screenY = attrY * 8;
        int scan = (screenY & B11000000) + (y << 3) + ((screenY & B111000) >> 3);
        uint8_t row = *(pixelBase + 32 * scan + attrX);
        uint8_t rowCopy = *(pixelBaseCopy + 32 * scan + attrX);
        // check for changes in the pixel data
        if (row != rowCopy)
        {
          dirty = true;
          *(pixelBaseCopy + 32 * scan + attrX) = row;
        }
        uint16_t *pixelAddress = pixelBuffer + 256 * y + attrX * 8;
        // Since the ESP32 is a 32-bit processor with a 32-bit memory bus,
        // it's more efficient to write 32-bits at a time. So...calculate
        // pairs of pixels and avoid conditional tests and branches.
        // Check for the 2 optimal cases of pure foreground or background
        if (row == 0) {
          uint32_t u32Clr = tftPaperColor | (tftPaperColor << 16);
          uint32_t *d32 = (uint32_t *)pixelAddress;
          *d32++ = u32Clr;
          *d32++ = u32Clr;
          *d32++ = u32Clr;
          *d32++ = u32Clr;
          pixelAddress += 8;
        } else if (row == 0xff) {
          uint32_t u32Clr = tftInkColor | (tftInkColor << 16);
          uint32_t *d32 = (uint32_t *)pixelAddress;
          *d32++ = u32Clr;
          *d32++ = u32Clr;
          *d32++ = u32Clr;
          *d32++ = u32Clr;
          pixelAddress += 8;
        } else { // Otherwise use a lookup table to write pairs of pixels
          uint32_t *d32 = (uint32_t *)pixelAddress;
          *d32++ = u32Lookup[row >> 6];
          *d32++ = u32Lookup[(row >> 4) & 3];
          *d32++ = u32Lookup[(row >> 2) & 3];
          *d32++ = u32Lookup[row & 3];
        }
      }
    }
    if (dirty || firstDraw)
    {
      if (!isShowingMenu || borderHeight + attrY * 8 < m_tft.height() - VOLUME_BAR_HEIGHT) { 
        m_tft.setWindow(borderWidth, borderHeight + attrY * 8, borderWidth + 255, borderHeight + attrY * 8 + 7);
        m_tft.pushPixels(pixelBuffer, 256 * 8);
      }
    }
  }
  drawReady = true;
  firstDraw = false;
  frameCount++;
  flashTimer++;
  if (flashTimer >= 32)
  {
    flashTimer = 0;
  }

}

void Renderer::drawTimeTravel() {
    m_tft.fillRect(0, 0, m_tft.width(), 20, TFT_BLACK);

    m_tft.loadFont(GillSans_15_vlw);
    m_tft.setTextColor(TFT_WHITE, TFT_BLACK);
    
    // Draw the left control "<5"
    m_tft.drawString("<5", 5, 0);
    
    // Draw the center text "Time Travel - Enter=Jump"
    Point centerSize = m_tft.measureString("Time Travel - Enter=Jump");
    int centerX = (m_tft.width() - centerSize.x) / 2;
    m_tft.drawString("Time Travel - Enter=Jump", centerX, 0);
    
    // Draw the right control "8>"
    Point rightSize = m_tft.measureString("8>");
    int rightX = m_tft.width() - rightSize.x - 5;  // 5 pixels from right edge
    m_tft.drawString("8>", rightX, 0);
}

void Renderer::drawMenu() {
    m_tft.fillRect(0, 0, m_tft.width(), MENU_BAR_HEIGHT, TFT_BLACK);

    m_tft.loadFont(GillSans_15_vlw);
    m_tft.setTextColor(TFT_WHITE, TFT_BLACK);
    
    Point menuSize = m_tft.measureString("1-Time Travel  2-Snapshot  ENTER-Resume");
    int centerX = (m_tft.width() - menuSize.x) / 2;
    m_tft.drawString("1-Time Travel  2-Snapshot  ENTER-Resume", centerX, 0);

    // Draw the volume control
    const char *volumeText = "<5       Volume       8>";
    Point volumeSize = m_tft.measureString(volumeText);
    int volumeX = (m_tft.width() - volumeSize.x) / 2;
    int volumeY = m_tft.height() - VOLUME_BAR_HEIGHT;

    m_tft.fillRect(0,volumeY, m_tft.width(), VOLUME_BAR_HEIGHT, TFT_BLACK);

    m_tft.drawString(volumeText, volumeX, volumeY + 5);

    int volumeRectX = 10;
    int volumeRectY = volumeY + volumeSize.y + 10;
    int volumeRectWidth = m_tft.width() - 20;
    int volumeRectHeight = 10;
    m_tft.fillRect(volumeRectX, volumeRectY, volumeRectWidth, volumeRectHeight, TFT_WHITE);
    int currentVolumeWidth = volumeRectWidth * m_audioOutput->getVolume() / 10;
    m_tft.fillRect(volumeRectX, volumeRectY, currentVolumeWidth, volumeRectHeight, TFT_GREEN);
}

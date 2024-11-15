#include "Renderer.h"
#include "../../emulator/spectrum.h"

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
    if (drawnBorderColors[borderPos + offset] != borderColor)
    {
      // Find consecutive lines with the same color
      int rangeStart = borderPos;
      while (borderPos < endPos && currentBorderColors[borderPos + offset] == borderColor &&
              drawnBorderColors[borderPos + offset] != borderColor)
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
  m_tft.startWrite();

  if (isLoading)
  {
    int position = loadProgress * screenWidth / 100;
    m_tft.fillRect(position, 0, screenWidth - position, 8, TFT_BLACK);
    m_tft.fillRect(0, 0, position, 8, TFT_GREEN);
  }
  int borderOffset = 36;

  // Draw the top border
  drawBorder(isLoading ? 8 : 0, borderHeight, borderOffset, screenWidth, screenWidth, borderHeight, false);

  // Draw the bottom border
  drawBorder(screenHeight - borderHeight, screenHeight, borderOffset, screenWidth, screenWidth, borderHeight, false);

  // Draw the left and right borders
  drawBorder(borderHeight, screenHeight - borderHeight, borderOffset, borderWidth, screenWidth, borderHeight, true);
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
      for (int y = 0; y < 8; y++)
      {
        // read the value of the pixels
        int screenY = attrY * 8 + y;
        int col = ((attrX * 8) & B11111000) >> 3;
        int scan = (screenY & B11000000) + ((screenY & B111) << 3) + ((screenY & B111000) >> 3);
        uint8_t row = *(pixelBase + 32 * scan + col);
        uint8_t rowCopy = *(pixelBaseCopy + 32 * scan + col);
        // check for changes in the pixel data
        if (row != rowCopy)
        {
          dirty = true;
          *(pixelBaseCopy + 32 * scan + col) = row;
        }
        uint16_t *pixelAddress = pixelBuffer + 256 * y + attrX * 8;
        for (int x = 0; x < 8; x++)
        {
          if (row & 128)
          {
            *pixelAddress = tftInkColor;
          }
          else
          {
            *pixelAddress = tftPaperColor;
          }
          pixelAddress++;
          row = row << 1;
        }
      }
    }
    if (dirty || firstDraw)
    {
      m_tft.setWindow(borderWidth, borderHeight + attrY * 8, borderWidth + 255, borderHeight + attrY * 8 + 7);
      m_tft.pushPixels(pixelBuffer, 256 * 8);
    }
  }
  m_tft.endWrite();
  drawReady = true;
  firstDraw = false;
  frameCount++;
  flashTimer++;
  if (flashTimer >= 32)
  {
    flashTimer = 0;
  }
}


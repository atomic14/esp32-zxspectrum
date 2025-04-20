#pragma once

#include "Screen.h"
#include "fonts/GillSans_25_vlw.h"
#include <vector>
#include <cmath>

class TestScreen : public Screen {
private:
  // HSV to RGB conversion (returns 8-bit per channel)
  void hsvToRgb(float h, float s, float v, uint8_t &r, uint8_t &g, uint8_t &b) {
    float c = v * s;
    float x = c * (1 - fabs(fmod(h / 60.0, 2) - 1));
    float m = v - c;
    float r1, g1, b1;
    if (h < 60)      { r1 = c; g1 = x; b1 = 0; }
    else if (h < 120)  { r1 = x; g1 = c; b1 = 0; }
    else if (h < 180)  { r1 = 0; g1 = c; b1 = x; }
    else if (h < 240)  { r1 = 0; g1 = x; b1 = c; }
    else if (h < 300)  { r1 = x; g1 = 0; b1 = c; }
    else               { r1 = c; g1 = 0; b1 = x; }
    r = (uint8_t)((r1 + m) * 255);
    g = (uint8_t)((g1 + m) * 255);
    b = (uint8_t)((b1 + m) * 255);
  }

  void drawHSVGradient() {
    m_tft.startWrite();
    int w = m_tft.width();
    int h = m_tft.height();
    std::vector<uint16_t> rowBuf(w);
    for (int y = 0; y < h; ++y) {
      float v = 1.0f;
      float s = 1.0f - (float)y / (h - 1);
      for (int x = 0; x < w; ++x) {
        float hue = 360.0f * x / (w - 1);
        uint8_t r, g, b;
        hsvToRgb(hue, s, v, r, g, b);
        rowBuf[x] = Display::swapBytes(Display::color565(r, g, b));
      }
      m_tft.setWindow(0, y, w - 1, y);
      m_tft.pushPixels(rowBuf.data(), w);
    }
    m_tft.endWrite();
  }

  // Generate a square wave buffer for a note (frequency in Hz, duration in ms)
  std::vector<uint8_t> generateNoteBuffer(float freq, int durationMs, int sampleRate = 15600) {
    int samples = (sampleRate * durationMs) / 1000;
    std::vector<uint8_t> buf(samples);
    int period = (int)(sampleRate / freq / 2);
    for (int i = 0; i < samples; ++i) {
      buf[i] = (i / period) % 2 ? 255 : 0;
    }
    return buf;
  }

  // Play a sequence of notes: 'a', 'b', 'c', 'd'
  void playNotes() {
    struct Note { float freq; int duration; };
    // Frequencies for notes a, b, c, d (approximate, in Hz)
    Note notes[] = {
      {440.0f, 180}, // A4
      {493.88f, 180}, // B4
      {523.25f, 180}, // C5
      {587.33f, 180}  // D5
    };
    if (!m_files->isAvailable(StorageType::SD)) {
      // Play notes in reverse order if SD card is not mounted
      for (int i = 3; i >= 0; --i) {
        auto buf = generateNoteBuffer(notes[i].freq, notes[i].duration);
        if (m_audioOutput) m_audioOutput->write(buf.data(), buf.size());
      }
    } else {
      // Play notes in normal order if SD card is mounted
      for (int i = 0; i < 4; ++i) {
        auto buf = generateNoteBuffer(notes[i].freq, notes[i].duration);
        if (m_audioOutput) m_audioOutput->write(buf.data(), buf.size());
      }
    }
  }

public:
  TestScreen(Display &tft, HDMIDisplay *hdmiDisplay, AudioOutput *audioOutput, IFiles *files)
    : Screen(tft, hdmiDisplay, audioOutput, files) {}

  void didAppear() override {
    drawHSVGradient();
    m_tft.loadFont(GillSans_25_vlw);
    std::string msg = "Test Screen and Audio";
    auto textSize = m_tft.measureString(msg.c_str());
    int textX = (m_tft.width() - textSize.x) / 2;
    int textY = 20;
    m_tft.fillRect(textX-5, textY-5, textSize.x+10, textSize.y+10, TFT_BLACK);
    m_tft.setTextColor(TFT_WHITE, TFT_BLACK);
    m_tft.drawString(msg.c_str(), textX, textY);

    // SD card status
    std::string sdMsg;
    uint16_t sdColor;
    if (m_files && m_files->isAvailable(StorageType::SD)) {
      sdMsg = "SD Card: Mounted";
      sdColor = TFT_GREEN;
    } else {
      sdMsg = "SD Card: Not Mounted";
      sdColor = TFT_RED;
    }
    auto sdSize = m_tft.measureString(sdMsg.c_str());
    int sdX = (m_tft.width() - sdSize.x) / 2;
    int sdY = m_tft.height()/2 - 20;
    m_tft.fillRect(sdX-5, sdY-5, sdSize.x+10, sdSize.y+10, TFT_BLACK);
    m_tft.setTextColor(sdColor, TFT_BLACK);
    m_tft.drawString(sdMsg.c_str(), sdX, sdY);

    std::string footer = "Press any key to return";
    auto footerSize = m_tft.measureString(footer.c_str());
    int footerX = (m_tft.width() - footerSize.x) / 2;
    int footerY = m_tft.height() - 40;
    m_tft.fillRect(footerX-5, footerY-5, footerSize.x+10, footerSize.y+10, TFT_BLACK);
    m_tft.setTextColor(TFT_WHITE, TFT_BLACK);
    m_tft.drawString(footer.c_str(), footerX, footerY);
    playNotes();
  }

  void pressKey(SpecKeys key) override {
    m_navigationStack->pop();
  }
}; 
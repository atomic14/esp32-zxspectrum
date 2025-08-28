#include "MessageScreen.h"
#include "NavigationStack.h"
#include "PokeScreen.h"
#include "Screen.h"
#include "../TFT/TFTDisplay.h"
#include "fonts/GillSans_25_vlw.h"
#include "fonts/GillSans_15_vlw.h"
#include "../Emulator/spectrum.h"
#include "../Emulator/snaps.h"

#include <sstream>

#define CURSOR_COLOR  TFT_CYAN

void PokeScreen::pressKey(SpecKeys key) 
{
  std::string ctrl = "";
  size_t l = 0;
  int16_t addr = 0;
  std::vector<byte> bytes;
  int base = 10;

  switch (key)
  {
  case JOYK_FIRE:
  case SPECKEY_ENTER:
    if (s_addr.length()<3 || s_data.length()==0){
      playErrorBeep();
      return;
    }
    addr = get_num(s_addr, 10);
    base = (s_addr[1]=='x') ? 16 : 10;
    bytes = get_bytes(s_data, base);
    for (int i=0; i<bytes.size(); i++){
      machine->z80_poke(addr+i, bytes[i]);
    }
    updateDisplay();
    break;
  case SPECKEY_H:
  {
    m_navigationStack->push(new MessageScreen("HELP", {
      "Enter the address and the data to be poked.",
      "If Addr starts with 0x values are hex, else decimal.",
      "Data is space separated.",
      "",
      "T - switch btween Addr and Data",
      "Z/Delete - Delete character",
      "Enter - poke data",
      "Q/Break - Quit",
      }, m_tft, m_hdmiDisplay, m_audioOutput));
    break;
  }
  case SPECKEY_Z:
  case SPECKEY_DEL:
    ctrl = getControlString();
    l = ctrl.length();
    if (l == 0){
      playErrorBeep();
      return;
    }
    ctrl.pop_back();
    setControlString(ctrl);
    updateDisplay();
    break;
  case SPECKEY_T:
    active_control = 1 - active_control;
    updateDisplay();
    break;
  case SPECKEY_Q:
  case SPECKEY_BREAK:
    m_navigationStack->pop();
    return;
  default:
    if (specKeyToLetter.find(key) == specKeyToLetter.end() && key != SPECKEY_SPACE)
      break;
    char chr = (key==SPECKEY_SPACE) ? ' ' : specKeyToLetter.at(key);
    if (chr == 'X'){
      bool bad = false;
      if (active_control == 1){
        // Only needs to be used for address. Bytes will be the same base
        bad = true;
      } else if (s_addr.length()!=1 || s_addr[0] != '0'){
        // Can only use after a 0 in the first position
        bad = true;
      }
      if (bad){
        playErrorBeep();
        return;
      }
      // Convert to lowercase
      s_addr.push_back('x');
      updateDisplay();
      return;
    }
    if (chr == ' '){
      // Can only be used to separate data
      if (active_control == 0){
        playErrorBeep();
        return;
      }
      s_data.push_back(chr);
      updateDisplay();
      return;
    }
    if (isHexadecimalDigit(chr)) {
      ctrl = getControlString();
      ctrl.push_back(chr);
      setControlString(ctrl);
      playKeyClick();
      updateDisplay();
      return;
    }
  }
};

void PokeScreen::updateDisplay()
{
  m_tft.loadFont(GillSans_15_vlw);
  m_tft.startWrite();
  m_tft.fillRect(0, 0, m_tft.width(), m_tft.height(), TFT_BLACK);
  m_tft.drawRect(0, 0, m_tft.width(), m_tft.height(), TFT_WHITE);
  m_tft.setTextColor(TFT_CYAN, TFT_BLACK);
  m_tft.drawString("Poke Screen (H for help)", 2, 2);

  m_tft.setTextColor(TFT_WHITE, TFT_BLACK);
  const char* c_addr = "Addr:";
  auto textSize = m_tft.measureString(c_addr);
  const int16_t edit_y = textSize.x + 7;
  int16_t cursor_x;

  m_tft.drawString(c_addr, 2, 20);
  cursor_x = m_tft.drawStringAndMeasure(s_addr.c_str(), edit_y, 20);
  if (active_control == 0){
    m_tft.fillRect(cursor_x, 20, 2, textSize.y, CURSOR_COLOR);
  }

  m_tft.drawString("Data:", 2, 40);
  cursor_x = m_tft.drawStringAndMeasure(s_data.c_str(), edit_y, 40);
  if (active_control == 1){
    m_tft.fillRect(cursor_x, 40, 2, textSize.y, CURSOR_COLOR);
  }

  uint16_t addr;
  if (s_addr.length()<3)
    addr = 0;
  else
    addr = get_num(s_addr, 10);

  char buf[16];
  bool is_hex = s_addr[1] == 'x';
  std::vector<byte> pokes;
  if (s_data.length()){
    // Base doesn't really matter here - doing this just to count the bytes
    pokes = get_bytes(s_data, 16);
  }

  uint16_t poke_len = pokes.size();
  if (poke_len == 0){
    poke_len = 1;
  }
  uint16_t start_addr = addr - 4;
  uint16_t end_addr = addr + poke_len + 4;

  m_tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  cursor_x = m_tft.drawStringAndMeasure("Bytes at ", 2, 60);
  if (is_hex)
    sprintf(buf, "%04X", addr);
  else
    sprintf(buf, "%d", addr);
  cursor_x = m_tft.drawStringAndMeasure(buf, cursor_x, 60);
  cursor_x = 2;

  for (int i=start_addr; i<end_addr; i++){
    auto b = machine->z80_peek(i);
    if (is_hex)
      sprintf(buf, "%02X", b);
    else
      sprintf(buf, "%d", b);
    if (i<addr || i>=addr+poke_len)
      m_tft.setTextColor(TFT_WHITE, TFT_BLACK);
    else
      m_tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    cursor_x = m_tft.drawStringAndMeasure(buf, cursor_x, 80) + 5;
  }

  m_tft.endWrite();
}

std::vector<std::string> PokeScreen::split(const std::string &s, char delim){
  std::vector<std::string> result;
  std::stringstream ss(s);
  std::string item;

  while (getline(ss, item, delim)){
    result.push_back(item);
  }
  return result;
}

// Parse a string as a number with the specified base.
// If the string starts with 0x the base is ignored and the 
// string is parsed as hex
int16_t PokeScreen::get_num(const std::string &s, int base){
  if (s[0] == '0' && s[1] == 'x'){
    base = 16;
  }
  return std::stoi(s, nullptr, base);
}

// Parse a string of bytes delimited by single spaces
std::vector<byte> PokeScreen::get_bytes(const std::string &s, int base){
  std::vector<byte> result;
  auto s_bytes = split(s, ' ');

  for (const std::string& s_byte: s_bytes){
    if (s_byte.length()==0)
      continue;
    auto b = get_num(s_byte, base) & 0xFF;
    result.push_back(b);
  }
  return result;
}

std::string PokeScreen::getControlString(){
  switch(active_control){
    case 0:
      return s_addr;
    default:
      return s_data;
  }
}

void PokeScreen::setControlString(std::string str){
  switch(active_control){
    case 0:
      s_addr = str;
      break;
    default:
      s_data = str;
      break;
  }
}
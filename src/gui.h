#pragma once

class TFT_eSPI;

#include "hardware.h"
#include "z80.h"

void gui_draw_window(TFT_eSPI &tft, int w, int h, const char *title);
void gui_update_menu(TFT_eSPI &tft, int w, int h, int n, int s, char data[][25]);
int gui_draw_menu(TFT_eSPI &tft, int w, const char *title, int n, char data[][25]);
void updatekey(TFT_eSPI &tft, tipo_emuopt &emuopt, Z80Regs &spectrumZ80, uint8_t key, uint8_t state);
void show_splash(TFT_eSPI &tft);
void gui_Main_Menu(TFT_eSPI &tft, tipo_emuopt &emuopt, Z80Regs &spectrumZ80);

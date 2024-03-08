/* GUI for the handheld edition of the emulator
 *  Gui for File selection
 *  Gui for hardware configuration
 *  - Change keymaps
 *  - Load a snapshot
 *  - Reset the emulator
 *  - define hardware emulation
 *  - define keymaps
 *  - volver el emulador
 *  Gui confirmacion
 */
/* Borrar la parte de la pantalla a utilizar
 *  Escribir un titulo de la "ventana"
 *  varias opciones a elegir
 *  scroll si son mas que las que caben en pantalla
 *  mover una barra o algo entre las opciones
 *  retornar si/no y el resultado en funcion de la seleccion
 */

// GENERIC FUNCTIONS:
#include <TFT_eSPI.h>
#include "gui.h"
#include "spectrum.h"
#include "snaps.h"

void gui_draw_window(TFT_eSPI &tft, int w, int h, const char *title)
{
  int x, y, n, m;
  x = (320 - w) / 2;
  y = (240 - h) / 2;
  n = x + w - 2;
  m = y + 8;
  tft.fillRect(x - 2, y - 2, w + 4, h + 4, tft.color565(64, 64, 64));
  tft.fillRect(x, y, w, 8, tft.color565(0, 0, 0));
  tft.fillRect(x + 1, y + 9, w - 2, h - 9, tft.color565(255, 255, 255));
  tft.drawRect(x, y + 8, w, h - 7, tft.color565(0, 0, 0));

  tft.setCursor(x + 2, y + 1);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(1);
  tft.println(title);

#ifdef USE_ADA
  tft.drawLine(n, y, (n--) - 8, m, tft.color565(0, 0, 255));
  tft.drawLine(n, y, (n--) - 8, m, tft.color565(0, 0, 255));
  tft.drawLine(n, y, (n--) - 8, m, tft.color565(0, 0, 255));
  tft.drawLine(n, y, (n--) - 8, m, tft.color565(0, 0, 255));
  tft.drawLine(n, y, (n--) - 8, m, tft.color565(0, 255, 0));
  tft.drawLine(n, y, (n--) - 8, m, tft.color565(0, 255, 0));
  tft.drawLine(n, y, (n--) - 8, m, tft.color565(0, 255, 0));
  tft.drawLine(n, y, (n--) - 8, m, tft.color565(0, 255, 0));
  tft.drawLine(n, y, (n--) - 8, m, tft.color565(255, 255, 0));
  tft.drawLine(n, y, (n--) - 8, m, tft.color565(255, 255, 0));
  tft.drawLine(n, y, (n--) - 8, m, tft.color565(255, 255, 0));
  tft.drawLine(n, y, (n--) - 8, m, tft.color565(255, 255, 0));
  tft.drawLine(n, y, (n--) - 8, m, tft.color565(255, 0, 0));
  tft.drawLine(n, y, (n--) - 8, m, tft.color565(255, 0, 0));
  tft.drawLine(n, y, (n--) - 8, m, tft.color565(255, 0, 0));
  tft.drawLine(n, y, (n--) - 8, m, tft.color565(255, 0, 0));
#else
  tft.drawLine(n, y, (n--) - 8, m, tft.color565(255, 0, 0));
  tft.drawLine(n, y, (n--) - 8, m, tft.color565(255, 0, 0));
  tft.drawLine(n, y, (n--) - 8, m, tft.color565(255, 0, 0));
  tft.drawLine(n, y, (n--) - 8, m, tft.color565(255, 0, 0));
  tft.drawLine(n, y, (n--) - 8, m, tft.color565(0, 255, 0));
  tft.drawLine(n, y, (n--) - 8, m, tft.color565(0, 255, 0));
  tft.drawLine(n, y, (n--) - 8, m, tft.color565(0, 255, 0));
  tft.drawLine(n, y, (n--) - 8, m, tft.color565(0, 255, 0));
  tft.drawLine(n, y, (n--) - 8, m, tft.color565(0, 255, 255));
  tft.drawLine(n, y, (n--) - 8, m, tft.color565(0, 255, 255));
  tft.drawLine(n, y, (n--) - 8, m, tft.color565(0, 255, 255));
  tft.drawLine(n, y, (n--) - 8, m, tft.color565(0, 255, 255));
  tft.drawLine(n, y, (n--) - 8, m, tft.color565(0, 0, 255));
  tft.drawLine(n, y, (n--) - 8, m, tft.color565(0, 0, 255));
  tft.drawLine(n, y, (n--) - 8, m, tft.color565(0, 0, 255));
  tft.drawLine(n, y, (n--) - 8, m, tft.color565(0, 0, 255));
#endif
}

void gui_update_menu(TFT_eSPI &tft, int w, int h, int n, int s, char data[][25])
{
  int c, x, y;
  x = (320 - w) / 2;
  y = (240 - h) / 2;

  tft.setTextColor(TFT_BLACK);
  tft.setTextSize(1);
  for (c = 1; c <= n; c++)
  {
#ifdef USE_ADA
    tft.fillRect(x + 1, y + 9 * c, w - 2, 9, (c == s ? TFT_CYAN : TFT_WHITE));
#else
    tft.fillRect(x + 1, y + 9 * c, w - 2, 9, (c == s ? TFT_YELLOW : TFT_WHITE));
#endif
    tft.setCursor(x + 2, y + 1 + 9 * c);
    tft.println(data[c - 1]);
  }
}
int gui_draw_menu(TFT_eSPI &tft, int w, const char *title, int n, char data[][25])
{
  int ret = -1;
  int h, b;
  int s = 1;
  h = 9 + 9 * (n > 7 ? 7 : n);
  gui_draw_window(tft, w, h, title);
  gui_update_menu(tft, w, h, n, s, data);
  do
  {
    b = mirabotones();
    switch (b)
    {
    case 4:
      if (s > 1)
      {
        s--;
        gui_update_menu(tft, w, h, n, s, data);
      }
      break;
    case 5:
      if (s < n)
      {
        s++;
        gui_update_menu(tft, w, h, n, s, data);
      }
      break;
    case 3:
      ret = 0;
      break;
    case 1:
      ret = s;
      break;
    }
  } while (ret == -1);
  return ret;
}

// SPECIFIC FUNCTIONS

void gui_Main_Menu(TFT_eSPI &tft, tipo_emuopt &emuopt, Z80Regs &spectrumZ80)
{
  char options[7][25] = {
      "Cambiar Mapa de teclado",
      "Cargar Snapshot",
      "Cargar Cinta",
      "Reset",
      "Tipo de Maquina",
      "Sonido ON/OFF",
      "Volver"};
  int r;
  r = gui_draw_menu(tft, 200, "Opciones", 7, options);
  switch (r)
  {
  case 1:
    emuopt.mappingindex++;
    if (emuopt.mappingindex > 3)
      emuopt.mappingindex = 0;
    AS_printf("now mapping is ");
    AS_print(emuopt.mappingindex);
    AS_printf("\n");
    break;
  case 4:
    reset_spectrum(&spectrumZ80);
    break;
  case 2:
    Load_SNA(&spectrumZ80, "/manic.sna");
    // gui_Select_Snap();
    break;
  case 6:
    if (emuopt.sonido == 1)
    {
      emuopt.sonido = 0;
      // TODO - handle sound
      // dac_output_voltage(DAC_CHANNEL_1, 0);
    }
    else
    {
      emuopt.sonido = 1;
    }
    break;
    /*
        case 3:
          gui_Select_Tape();
          break;
    */
  case 5:
    //      gui_Select_Machine();
    show_splash(tft);
    do
    {
      sleep(10);
    } while (mirabotones() == 0);
    break;
  }
  tft.fillScreen(TFT_BLACK);
  // tft.fillRect(0,0,320,240,tft.color565(0,0,0));
  // for (r = 0; r < (320 * 240); r++)
  // {
  //   *(lastpix + r) = 0;
  // }
}

void show_splash(TFT_eSPI &tft)
{

  gui_draw_window(tft, 270, 170, "Version 0.0 alpha");
  tft.setCursor(0, 65);
  tft.setTextColor(TFT_BLACK);
  tft.setTextSize(3);
  tft.println("    OpenVEGA+");
  tft.setTextColor(TFT_DARKGREEN);
  tft.setTextSize(2);
  tft.println("\n     By Alvaro Alea F.");
  tft.setTextSize(1);
  tft.println("");
  tft.setTextSize(2);
  tft.println("          (C) 2019");
  tft.setTextSize(1);
  tft.setTextColor(TFT_BLUE);
  tft.println("\n\n\n       A Opensource Portable Spectrum Emulator");
}

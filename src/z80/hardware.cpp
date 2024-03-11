/*=====================================================================
 * Open Vega+ Emulator. Handheld ESP32 based hardware with 
 * ZX Spectrum emulation
 * 
 * (C) 2019 Alvaro Alea Fernandez
 *
 * This program is free software; redistributable under the terms of 
 * the GNU General Public License Version 2
 * 
 * Based on the Aspectrum emulator Copyright (c) 2000 Santiago Romero Iglesias
 *======================================================================
 *
 * This file contains the Real Hardware to Emulate Hardware part
 *
 *======================================================================
 */
#include <Arduino.h>
#include "hardware.h"

uint8_t kempston_port=0x0;
uint8_t ulaport_FF=0xFF;

// Con estas variables se controla el mapeado de los botones  hardware reales a los virtuales del spectrum
uint8_t mappingkey[4][12]={  
{SPECKEY_Z, SPECKEY_M,SPECKEY_SPACE, SPECKEY_ENTER, SPECKEY_Q, SPECKEY_A, SPECKEY_O, SPECKEY_P,  VEGAKEY_MENU,SPECKEY_SHIFT,SPECKEY_J,SPECKEY_H},
{SPECKEY_P, JOYK_FIRE,SPECKEY_SPACE, SPECKEY_ENTER, JOYK_UP,   JOYK_DOWN, JOYK_LEFT, JOYK_RIGHT, VEGAKEY_MENU,SPECKEY_SHIFT,SPECKEY_J,SPECKEY_H},
{SPECKEY_P, SPECKEY_0,SPECKEY_SPACE, SPECKEY_ENTER, SPECKEY_9, SPECKEY_8, SPECKEY_6, SPECKEY_7,  VEGAKEY_MENU,SPECKEY_SHIFT,SPECKEY_J,SPECKEY_H},
{SPECKEY_5, SPECKEY_M,SPECKEY_SPACE, SPECKEY_ENTER, SPECKEY_P, SPECKEY_L, SPECKEY_Z, SPECKEY_X,  VEGAKEY_MENU,SPECKEY_SHIFT,SPECKEY_J,SPECKEY_H},
};


// Con estas variables se controla el mapeado de las teclas virtuales del spectrum a I/O port
const int key2specy[2][41]={
  { 0, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4,
       2, 2, 2, 2, 2, 5, 5, 5, 5, 5,
       1, 1, 1, 1, 1, 6, 6, 6, 6, 6,
       0, 0, 0, 0, 0, 7, 7, 7, 7, 7 },
  { 0, 0xFE, 0xFD, 0xFB, 0xF7, 0xEF, 0xEF, 0xF7, 0xFB, 0xFD, 0xFE,
       0xFE, 0xFD, 0xFB, 0xF7, 0xEF, 0xEF, 0xF7, 0xFB, 0xFD, 0xFE,
       0xFE, 0xFD, 0xFB, 0xF7, 0xEF, 0xEF, 0xF7, 0xFB, 0xFD, 0xFE,
       0xFE, 0xFD, 0xFB, 0xF7, 0xEF, 0xEF, 0xF7, 0xFB, 0xFD, 0xFE }
};
uint8_t speckey[8]={0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};

int keys[16]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
int oldkeys[16]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

void updatekey(uint8_t key, uint8_t state){
  uint8_t n;
  // Bit pattern: XXXFULDR
  switch (key) {
    case JOYK_RIGHT:
      if (state==1) kempston_port |= B00000001;
      else       kempston_port &= B11111110;
      break;
    case JOYK_LEFT:
      if (state==1) kempston_port |= B00000010;
      else       kempston_port &= B11111101;
      break;
    case JOYK_DOWN:
      if (state==1) kempston_port |= B00000100;
      else       kempston_port &= B11111011;
      break;
    case JOYK_UP:
      if (state==1) kempston_port |= B00001000;
      else       kempston_port &= B11110111;
      break;
    case JOYK_FIRE:
      if (state==1) kempston_port |= B00010000;
      else       kempston_port &= B11101111;
      break;
    default:
      if (state==1) n=  key2specy[1][key] ;
      else          n= (key2specy[1][key])^0xFF ;

      if (state==1) speckey[ key2specy[0][key] ] &=  key2specy[1][key] ;
      else          speckey[ key2specy[0][key] ] |= ((key2specy[1][key])^0xFF) ;
      break;
  }
}

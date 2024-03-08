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
#include <TFT_eSPI.h>
#include "hardware.h"
#include "gui.h"

#include <Wire.h>

//FIXME part of struct of speccy???
uint8_t border=0;
uint8_t kempston_port=0xFF;
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

// MAPA conlos 8 INS + KEMPSTON
// MAPA con 8 botones  a >teclas
// mapeamos hardware a teclas
// mapeamos teclas a INS

void leebotones(TFT_eSPI &tft, tipo_emuopt &emuopt, Z80Regs &spectrumZ80, uint8_t *lastpix){
  // TODO - reading buttons
  return;
//   int c;
//   keypad_i2c_read();
//   for(c=0;c<12;c++){
//   if (keys[c]!=oldkeys[c]) {
//     AS_printf("B="); AS_print(c);
//     AS_printf("\tM="); AS_print(mappingkey[emuopt.mappingindex][c]);
//     AS_printf("\n");
//     updatekey(tft, emuopt, spectrumZ80, lastpix, mappingkey[emuopt.mappingindex][c], keys[c]);
// //    paint_tecla (mappingkey[emuopt.mappingindex][c],keys[c]);
// //    paint_button(c,keys[c]);
//   }
//   oldkeys[c]=keys[c];  
//   }

}

int mirabotones(void){
  // TODO - reading buttons
  // int c;
  // int r=-1;
  // static int tmp_oldkeys[16]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
  // keypad_i2c_read();
  // for(c=0;c<12;c++){
  // if (keys[c]!=tmp_oldkeys[c]) {
  //   if (keys[c]==0) r=c;
  // }
  // tmp_oldkeys[c]=keys[c];  
  // }
  // return r;
  return -1;
}

void updatekey(TFT_eSPI &tft, tipo_emuopt &emuopt, Z80Regs &spectrumZ80, uint8_t *lastpix, uint8_t key, uint8_t state){
  uint8_t n;
  switch (key) {
    case SPECKEY_NONE :
      break;
    case VEGAKEY_MENU :
      if (state==1) gui_Main_Menu(tft, emuopt, spectrumZ80, lastpix);
      break;
    case JOYK_LEFT :
      if (state==1) kempston_port &=!0x01;
      else       kempston_port |= 0x01;   
      break;
    case JOYK_RIGHT :
      if (state==1) kempston_port &=!0x02;
      else       kempston_port |= 0x02;   
      break;
    case JOYK_UP :
      if (state==1) kempston_port &=!0x04;
      else       kempston_port |= 0x04;   
      break;
    case JOYK_DOWN :
      if (state==1) kempston_port &=!0x08;
      else       kempston_port |= 0x08;   
      break;
    case JOYK_FIRE :
      if (state==1) kempston_port &=!0x10;
      else       kempston_port |= 0x10;   
      break;
    default:
      if (state==1) n=  key2specy[1][key] ;
      else          n= (key2specy[1][key])^0xFF ;

      if (state==1) speckey[ key2specy[0][key] ] &=  key2specy[1][key] ;
      else          speckey[ key2specy[0][key] ] |= ((key2specy[1][key])^0xFF) ;
//      AS_printf("Fila %i, state %i, es:",key2specy[0][key],state); AS_print(speckey[ key2specy[0][key] ],BIN);
//      AS_printf(" sacado de:"); AS_print(n,BIN); 
//      AS_print("\n");
      break;
  }
}




void paint_tecla(TFT_eSPI &tft, int b, int state){
const uint8_t k[5][48]= {
  {0, 255,255,255,255,255, 255,255,255,255,255, 
      255,255,255,255,255, 255,255,255,255,255, 
      255,255,255,255,255, 255,255,255,255,255,
      255,255,255,255,255, 255,255,255,255,255,
      0,  0,  0,  0,  0,   255 },
  {0, 0,  0,  0,  0,  0,   0,  0,  0,  0,  0,   
      0,  0,  0,  0,  0,   0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,   0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,   0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,   0 },
  {0, 0,  0,  0,  0,  0,   0,  0,  0,  0,  0,   
      0,  0,  0,  0,  0,   0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,   0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,   0,  0,  0,  0,  0,
      255,255,255,255,255, 255 },
  {0, 0,  5,  10, 15, 20,  25, 30, 35, 40, 45,   
      0,  5,  10, 15, 20,  25, 30, 35, 40, 45,
      0,  5,  10, 15, 20,  25, 30, 35, 40, 45,
      0,  5,  10, 15, 20,  25, 30, 35, 40, 45,
      65, 65, 60, 70, 75,  50 },
  {0, 220,220,220,220,220, 220,220,220,220,220,   
      225,225,225,225,225, 225,225,225,225,225,
      230,230,230,230,230, 230,230,230,230,230,
      235,235,235,235,235, 235,235,235,235,235,
      220,230,225,225,235, 235 }
  };

  if (b!=SPECKEY_NONE) {
    if (state==0) {
      tft.fillRect(k[3][b],k[4][b],4,3,tft.color565(0,0,0));
    } else {
      tft.fillRect(k[3][b],k[4][b],4,3,tft.color565(k[0][b],k[1][b],k[2][b]));
    }  
  }
}
void paint_button(TFT_eSPI &tft, int b, int state){
const  uint8_t k[5][12]= { 
    { 255,255,  0,  0,  255,255,255,255,   0,255,128,128},
    {   0,255,255,  0,  255,255,255,255, 255,  0,128,  0},
    {   0,  0,  0,255,  255,255,255,255, 255,255,128,  0},
    { 30,35,30,25,      10, 10, 5, 15,      35,35,30,25},
    {  9,12,15,12,      9,15,12,12,      3,19,19,19}
  };


  if (state==0) {
    tft.fillRect(k[3][b],k[4][b],4,3,tft.color565(0,0,0));
  } else {
    tft.fillRect(k[3][b],k[4][b],4,3,tft.color565(k[0][b],k[1][b],k[2][b]));
  }

}

// Function for writing two Bytes to the I2C expander device
void pf575_write(uint16_t dato)
{
  Wire.beginTransmission(KEYPADADRESS);
  Wire.write(lowByte(dato));
  Wire.write(highByte(dato));
  Wire.endTransmission();
}

uint16_t pf575_read(void){
  uint8_t a,b;
  Wire.requestFrom(KEYPADADRESS, 2); //request two bytes of data
  if (Wire.available()) { //really this do anything??
    a = Wire.read(); //read byte 1
    b = Wire.read(); //read byte 2
  }
  return (uint16_t)b + (uint16_t)a<<8 ;
}

void keypad_i2c_read(void){
  uint8_t dataReceived[3]; //a two byte array to hold our data
  int c;
  pf575_write(0xFFFE);
//  dataReceived[1]=lowByte(pf575_read());
  Wire.requestFrom(KEYPADADRESS, 2); //request two bytes of data
  if (Wire.available()) {
    dataReceived[0] = Wire.read(); //read byte 1
    dataReceived[1] = Wire.read(); //read byte 2
  }
  pf575_write(0xFFFD);
//  dataReceived[2]=lowByte(pf575_read());
  Wire.requestFrom(KEYPADADRESS, 2); //request two bytes of data
  if (Wire.available()) {
    dataReceived[0] = Wire.read(); //read byte 1
    dataReceived[2] = Wire.read(); //read byte 2
  }
  pf575_write(0xFFFF);

  for(c=0;c<8;c++){
    keys[c]=!(dataReceived[1] & 1);
    dataReceived[1]=dataReceived[1]>>1;
  }
  for(c=8;c<12;c++){
    keys[c]=!(dataReceived[2] & 1);
    dataReceived[2]=dataReceived[2]>>1;
  }
}

void keypad_i2c_init(void){
  // TODO - buttons
  // Wire.begin();
  // pf575_write(0xFFFF);
}

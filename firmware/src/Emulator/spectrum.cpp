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
 * This file contains the specific ZX Spectrum part
 *
 *======================================================================
 */
#include <stdint.h>
#include <stdlib.h>
#include <SPIFFS.h>
#include "spectrum.h"
#include "AudioOutput/AudioOutput.h"
#include "Input/TouchKeyboard.h"
#include "48k_rom.h"
#include "128k_rom.h"

// Con estas variables se controla el mapeado de las teclas virtuales del spectrum a I/O port
const int key2specy[2][41] = {
    {0, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4,
     2, 2, 2, 2, 2, 5, 5, 5, 5, 5,
     1, 1, 1, 1, 1, 6, 6, 6, 6, 6,
     0, 0, 0, 0, 0, 7, 7, 7, 7, 7},
    {0, 0xFE, 0xFD, 0xFB, 0xF7, 0xEF, 0xEF, 0xF7, 0xFB, 0xFD, 0xFE,
     0xFE, 0xFD, 0xFB, 0xF7, 0xEF, 0xEF, 0xF7, 0xFB, 0xFD, 0xFE,
     0xFE, 0xFD, 0xFB, 0xF7, 0xEF, 0xEF, 0xF7, 0xFB, 0xFD, 0xFE,
     0xFE, 0xFD, 0xFB, 0xF7, 0xEF, 0xEF, 0xF7, 0xFB, 0xFD, 0xFE}};
uint8_t speckey[8] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

int keys[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int oldkeys[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

ZXSpectrum::ZXSpectrum()
{
  z80Regs = (Z80Regs *)malloc(sizeof(Z80Regs));
  z80Regs->userInfo = this;
}

void ZXSpectrum::reset()
{
  Z80Reset(z80Regs);
  Z80FlagTables();
}

void ZXSpectrum::runForCycles(int cycles)
{
  Z80Run(z80Regs, cycles);
}

int ZXSpectrum::runForFrame(AudioOutput *audioOutput, FILE *audioFile)
{
  int8_t audioBuffer[312];
  uint8_t *attrBase = mem.currentScreen + 0x1800;
  int c = 0;
  // Each line should be 224 tstates long...
  // And a complete frame is (64+192+56)*224=69888 tstates long
  for (int i = 0; i < 312; i++)
  {
    // handle port FF for the border
    if (i < 64 || i >= 192 + 64)
    {
      hwopt.portFF = 0xFF;
    }
    else
    {
      // otherwise we need to populate it with the correct attribute color
      uint8_t attr = *(attrBase + 32 * (i - 64) / 8);
      hwopt.portFF = attr;
    }
    // run for 224 cycles
    c += 224;
    runForCycles(224);
    if (hwopt.SoundBits != 0)
    {
      audioBuffer[i] = 20;
    }
    else
    {
      audioBuffer[i] = -20;
    }
  }
  interrupt();
  // write the audio buffer to the I2S device - this will block if the buffer is full which will control our frame rate 312/15.6KHz = 1/50th of a second
  audioOutput->write(audioBuffer, 312);
  // if (audioFile != NULL) {
  //   fwrite(audioBuffer, 1, 312, audioFile);
  //   fflush(audioFile);
  // }
  return c;
}

void ZXSpectrum::interrupt()
{
  // TODO - what should the 0x38 actually be?
  Z80Interrupt(z80Regs, 0x38);
}

void ZXSpectrum::updatekey(SpecKeys key, uint8_t state)
{
  // Bit pattern: XXXFULDR
  switch (key)
  {
  case JOYK_RIGHT:
    if (state == 1)
      kempston_port |= B00000001;
    else
      kempston_port &= B11111110;
    break;
  case JOYK_LEFT:
    if (state == 1)
      kempston_port |= B00000010;
    else
      kempston_port &= B11111101;
    break;
  case JOYK_DOWN:
    if (state == 1)
      kempston_port |= B00000100;
    else
      kempston_port &= B11111011;
    break;
  case JOYK_UP:
    if (state == 1)
      kempston_port |= B00001000;
    else
      kempston_port &= B11110111;
    break;
  case JOYK_FIRE:
    if (state == 1)
      kempston_port |= B00010000;
    else
      kempston_port &= B11101111;
    break;
  default:
    if (state == 1)
      speckey[key2specy[0][key]] &= key2specy[1][key];
    else
      speckey[key2specy[0][key]] |= ((key2specy[1][key]) ^ 0xFF);
    break;
  }
}

uint8_t ZXSpectrum::z80_in(uint16_t port)
{
  // Read from the ULA - basically keyboard
  if ((port & 0x01) == 0)
  {
    uint8_t data = 0xFF;
    if (!(port & 0x0100))
      data &= speckey[0]; // keys shift,z-v
    if (!(port & 0x0200))
      data &= speckey[1]; // keys a-g
    if (!(port & 0x0400))
      data &= speckey[2]; // keys q-t
    if (!(port & 0x0800))
      data &= speckey[3]; // keys 1-5
    if (!(port & 0x1000))
      data &= speckey[4]; // keys 6-0
    if (!(port & 0x2000))
      data &= speckey[5]; // keys y-p
    if (!(port & 0x4000))
      data &= speckey[6]; // keys h-l,enter
    if (!(port & 0x8000))
      data &= speckey[7]; // keys b-m,symb,space
    return data;
  }
  // kempston joystick
  if ((port & 0x01F) == 0x1F)
  {
    return kempston_port;
  }

  // emulacion port FF
  if ((port & 0xFF) == 0xFF)
  {
    if (!hwopt.emulate_FF)
      return 0xFF;
    else
    {
      return hwopt.portFF;
    }
  }
  return 0xFF;
}

void ZXSpectrum::z80_out(uint16_t port, uint8_t data)
{
  if (!(port & 0x01))
  {
    hwopt.BorderColor = (data & 0x07);
    hwopt.SoundBits = (data & B00010000);
  }
  else
  {
    // check for AY chip
    if ((port & 0x8002) == 0x8000)
    {
      // TODO - AY emulation
      return;
    }
    if ((port & 0x8002) == 0)
    {
      // paging
      mem.page(data);
    }
  }
}

bool ZXSpectrum::init_spectrum(int model)
{
  switch (model)
  {
  // only 48K supported for now
  case SPECMDL_48K:
    init_48k();
    return true;
  case SPECMDL_128K:
    init_128k();
    return true;
  }
  return false;
}

/* This do aditional stuff for reset, like mute sound chip, o reset bank switch */
void ZXSpectrum::reset_spectrum(Z80Regs *regs)
{
  if (hwopt.hw_model == SPECMDL_128K)
  {
    reset_128k();
  }
  Z80Reset(regs);
}

bool ZXSpectrum::init_48k()
{
  // ULA config
  hwopt.emulate_FF = 1;
  hwopt.ts_lebo = 24;    // left border t states
  hwopt.ts_grap = 128;   // graphic zone t states
  hwopt.ts_ribo = 24;    // right border t states
  hwopt.ts_hore = 48;    // horizontal retrace t states
  hwopt.ts_line = 224;   // to speed the calc, the sum of 4 abobe
  hwopt.line_poin = 16;  // lines in retraze post interrup
  hwopt.line_upbo = 48;  // lines of upper border
  hwopt.line_grap = 192; // lines of graphic zone = 192
  hwopt.line_bobo = 48;  // lines of bottom border
  hwopt.line_retr = 8;   // lines of the retrace
  hwopt.TSTATES_PER_LINE = 224;
  hwopt.TOP_BORDER_LINES = 64;
  hwopt.SCANLINES = 192;
  hwopt.BOTTOM_BORDER_LINES = 56;
  hwopt.tstate_border_left = 24;
  hwopt.tstate_graphic_zone = 128;
  hwopt.tstate_border_right = 72;
  hwopt.hw_model = SPECMDL_48K;
  hwopt.int_type = NORMAL;
  hwopt.SoundBits = 1;
  mem.loadRom(ZXSpectrum_48_rom, ZXSpectrum_48_rom_len);
  return true;
}

bool ZXSpectrum::init_16k()
{
  // ULA config
  hwopt.emulate_FF = 1;
  hwopt.ts_lebo = 24;    // left border t states
  hwopt.ts_grap = 128;   // graphic zone t states
  hwopt.ts_ribo = 24;    // right border t states
  hwopt.ts_hore = 48;    // horizontal retrace t states
  hwopt.ts_line = 224;   // to speed the calc, the sum of 4 abobe
  hwopt.line_poin = 16;  // lines in retraze post interrup
  hwopt.line_upbo = 48;  // lines of upper border
  hwopt.line_grap = 192; // lines of graphic zone = 192
  hwopt.line_bobo = 48;  // lines of bottom border
  hwopt.line_retr = 8;   // lines of the retrace
  hwopt.TSTATES_PER_LINE = 224;
  hwopt.TOP_BORDER_LINES = 64;
  hwopt.SCANLINES = 192;
  hwopt.BOTTOM_BORDER_LINES = 56;
  hwopt.tstate_border_left = 24;
  hwopt.tstate_graphic_zone = 128;
  hwopt.tstate_border_right = 72;
  hwopt.hw_model = SPECMDL_16K;
  hwopt.int_type = NORMAL;
  hwopt.SoundBits = 1;
  // treat it like a 48K
  mem.loadRom(ZXSpectrum_48_rom, ZXSpectrum_48_rom_len);
  return true;
}

bool ZXSpectrum::init_128k()
{
  // ULA config
  hwopt.emulate_FF = 1;
  hwopt.ts_lebo = 24;    // left border t states
  hwopt.ts_grap = 128;   // graphic zone t states
  hwopt.ts_ribo = 24;    // right border t states
  hwopt.ts_hore = 52;    // horizontal retrace t states
  hwopt.ts_line = 228;   // to speed the calc, the sum of 4 abobe
  hwopt.line_poin = 15;  // lines in retraze post interrup
  hwopt.line_upbo = 48;  // lines of upper border
  hwopt.line_grap = 192; // lines of graphic zone = 192
  hwopt.line_bobo = 48;  // lines of bottom border
  hwopt.line_retr = 8;   // lines of the retrace
  hwopt.TSTATES_PER_LINE = 228;
  hwopt.TOP_BORDER_LINES = 64;
  hwopt.SCANLINES = 192;
  hwopt.BOTTOM_BORDER_LINES = 56;
  hwopt.tstate_border_left = 24;
  hwopt.tstate_graphic_zone = 128;
  hwopt.tstate_border_right = 80;
  hwopt.hw_model = SPECMDL_128K;
  hwopt.int_type = NORMAL;
  hwopt.SoundBits = 1;
  mem.loadRom(ZXSpectrum_128_rom, ZXSpectrum_128_rom_len);
  return true;
}

void ZXSpectrum::reset_128k(void)
{
  mem.page(0x00);
}

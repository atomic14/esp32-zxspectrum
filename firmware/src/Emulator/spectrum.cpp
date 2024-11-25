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
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "../AudioOutput/AudioOutput.h"
#include "spectrum.h"
#include "48k_rom.h"
#include "128k_rom.h"

const uint16_t specpal565[16] = {
    0x0000, 0x1B00, 0x00B8, 0x17B8, 0xE005, 0xF705, 0xE0BD, 0x18C6, 0x0000, 0x1F00, 0x00F8, 0x1FF8, 0xE007, 0xFF07, 0xE0FF, 0xFFFF
};


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

int ZXSpectrum::runForFrame(AudioOutput *audioOutput, FILE *audioFile)
{
  uint8_t audioBuffer[312];
  uint8_t *attrBase = mem.currentScreen->data + 0x1800;
  int c = 0;
  // Each line should be 224 tstates long...
  // And a complete frame is (64+192+56)*224=69888 tstates long
  for (int i = 0; i < 312; i++)
  {
    if (audioOutput) {
      setMicValue(audioOutput->getMicValue());
    }
    // handle port FF for the border - is this actually doing anything useful?
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
    uint16_t speakerValue = runForCycles(224);
    borderColors[i] = hwopt.BorderColor & 0b00000111;
    audioBuffer[i] = speakerValue/4;
  }
  interrupt();

  // AY emulation
  if (hwopt.hw_model == SPECMDL_128K)
  {
    AySound::gen_sound(312, 0);
    // merge the AY sound with the audio buffer
    for (int i = 0; i < 312; i++)
    {
      // max output from the AYSound is 158 (I think...), scale it up to 255 and combine with the buzzer output
      audioBuffer[i] = std::max((int) audioBuffer[i], (int) AySound::SamplebufAY[i] * 255 / 158);
    }
  }
  if (audioFile != NULL) {
    fwrite(audioBuffer, 1, 312, audioFile);
    fflush(audioFile);
  }
  // write the audio buffer to the I2S device - this will block if the buffer is full which will control our frame rate 312/15.6KHz = 1/50th of a second
  if (audioOutput) {
    audioOutput->write(audioBuffer, 312);
  }
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
      kempston_port |= 0b00000001;
    else
      kempston_port &= 0b11111110;
    break;
  case JOYK_LEFT:
    if (state == 1)
      kempston_port |= 0b00000010;
    else
      kempston_port &= 0b11111101;
    break;
  case JOYK_DOWN:
    if (state == 1)
      kempston_port |= 0b00000100;
    else
      kempston_port &= 0b11111011;
    break;
  case JOYK_UP:
    if (state == 1)
      kempston_port |= 0b00001000;
    else
      kempston_port &= 0b11110111;
    break;
  case JOYK_FIRE:
    if (state == 1)
      kempston_port |= 0b00010000;
    else
      kempston_port &= 0b11101111;
    break;
  default:
    if (state == 1)
      speckey[key2specy[0][key]] &= key2specy[1][key];
    else
      speckey[key2specy[0][key]] |= ((key2specy[1][key]) ^ 0xFF);
    break;
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
  hwopt.SoundBits = 0;
  mem.page(32, true);
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
  hwopt.SoundBits = 0;
  // treat it like a 48K
  mem.page(32, true);
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
  hwopt.SoundBits = 0;
  mem.page(0, true);
  mem.loadRom(ZXSpectrum_128_rom, ZXSpectrum_128_rom_len);

  // setup the AYSound emulator
  printf("Setting up AySound");
  AySound::init();
  AySound::set_sound_format(15625,1,8);
  AySound::set_stereo(AYEMU_MONO,NULL);
  AySound::reset();

  // Empty audio buffers
  for (int i=0;i<SAMPLES_PER_FRAME;i++) {
    AySound::SamplebufAY[i]=0;
  }
  return true;
}

void ZXSpectrum::reset_128k(void)
{
  mem.page(0x00, true);
}

#ifndef SPECTRUM_H
#define SPECTRUM_H

#include <stdint.h>
#include "z80/z80.h"
#include "keyboard_defs.h"
#include <string.h>
#include "../AYSound/AySound.h"

extern uint8_t speckey[8];

extern const uint16_t specpal565[16];

enum models_enum
{
  SPECMDL_UNKNOWN = 0,
  SPECMDL_16K = 1,
  SPECMDL_48K,
  // SPECMDL_INVES,
  SPECMDL_128K,
  // SPECMDL_PLUS2,
  // SPECMDL_PLUS3,
  // SPECMDL_48KIF1,
  // SPECMDL_48KTRANSTAPE
};

enum inttypes_enum
{
  NORMAL = 1,
  INVES
};

typedef struct
{
  int ts_lebo;   // left border t states
  int ts_grap;   // graphic zone t states
  int ts_ribo;   // right border t states
  int ts_hore;   // horizontal retrace t states
  int ts_line;   // to speed the calc, the sum of 4 abobe
  int line_poin; // lines in retraze post interrup
  int line_upbo; // lines of upper border
  int line_grap; // lines of graphic zone = 192
  int line_bobo; // lines of bottom border
  int line_retr; // lines of the retrace
  int TSTATES_PER_LINE;
  int TOP_BORDER_LINES;
  int SCANLINES;
  int BOTTOM_BORDER_LINES;
  int tstate_border_left;
  int tstate_graphic_zone;
  int tstate_border_right;
  models_enum hw_model;
  int int_type;
  int emulate_FF;
  uint8_t BorderColor;
  uint8_t portFF;
  uint8_t SoundBits;
} tipo_hwopt;

class Memory {
  public:
    uint8_t hwBank = 0;
    uint8_t *rom[2];
    uint8_t *banks[8];
    uint8_t *currentScreen;
    uint8_t *mappedMemory[4];
    Memory() {
      // allocate space for the rom
      for (int i = 0; i < 2; i++) {
        rom[i] = (uint8_t *)malloc(0x4000);
        if (rom[i] == 0) {
          printf("Failed to allocate ROM");
        }
        memset(rom[i], 0, 0x4000);
      }
      // allocate space for the memory banks
      for (int i = 0; i < 8; i++) {
        banks[i] = (uint8_t *)malloc(0x4000);
        if (banks[i] == 0) {
          printf("Failed to allocate RAM");
        }
        memset(banks[i], 0, 0x4000);
      }
      // wire up the default memory configuration - this will work for the 48k model and is the default for the 128k model
      mappedMemory[0] = rom[0];
      mappedMemory[1] = banks[5];
      mappedMemory[2] = banks[2];
      mappedMemory[3] = banks[0];
      currentScreen = banks[5];
    }
    // handle the 128k paging
    void page(uint8_t newHwBank, bool force = false) {
      // check to see if paging has been disabled
      if ((hwBank & 32) && !force) {
        return;
      }
      hwBank = newHwBank;
      // the lower 3 bits of the bank register determine which ram bank is paged in to the top 16K
      mappedMemory[3] = banks[hwBank & 0x07];
      // bit 3 controls the video page - but this is just for the ULA, the CPU always sees the same memory
      currentScreen = banks[hwBank & 0x08 ? 7 : 5];
      // bit 4 of the bank register determines which rom bank is paged in
      mappedMemory[0] = rom[hwBank & 0x10 ? 1 : 0];      
    }
    inline uint8_t peek(int address) {
      int memoryBank = address >> 14;
      int bankAddress = address & 0x3fff;
      return mappedMemory[memoryBank][bankAddress];
    }
    inline void poke(int address, uint8_t value) {
      int memoryBank = address >> 14;
      int bankAddress = address & 0x3fff;
      if (memoryBank == 0) {
        // ignore writes to rom
      } else {
        mappedMemory[memoryBank][bankAddress] = value;
      }
    }
    void loadRom(const uint8_t *rom_data, int rom_len) {
      printf("Loading ROM %d\n", rom_len);
      int romCount = rom_len / 0x4000;
      for (int i = 0; i < romCount; i++) {
        printf("Copying ROM %d\n", i);
        memcpy(rom[i], rom_data + (i * 0x4000), 0x4000);
      }
    }
};

class AudioOutput;

class ZXSpectrum
{
public:
  Z80Regs *z80Regs;
  Memory mem;
  tipo_hwopt hwopt = {};
  uint8_t kempston_port = 0x0;
  uint8_t ulaport_FF = 0xFF;
  bool micLevel = false;
  uint8_t borderColors[312] = {0};

  ZXSpectrum();
  void reset();
  int runForFrame(AudioOutput *audioOutput, FILE *audioFile);
  inline int runForCycles(int cycles)
  {
    return Z80Run(z80Regs, cycles);
  }

  void interrupt();
  void updatekey(SpecKeys key, uint8_t state);

  inline uint8_t z80_peek(uint16_t address)
  {
    return mem.peek(address);
  }

  inline void z80_poke(uint16_t address, uint8_t value)
  {
    mem.poke(address, value);
  }

inline uint8_t z80_in(uint16_t port)
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

    // set bit 6 if the MIC is active
    if (micLevel)
    {
      data |= 0x40;
    }
    else
    {
      data &= 0xBF;
    }
    return data;
  }
  // kempston joystick
  if ((port & 0x01F) == 0x1F)
  {
    return kempston_port;
  }
  if (hwopt.hw_model == SPECMDL_128K) {
    if ((port & 0xC002) == 0xC000) {
      return AySound::getRegisterData();
    }
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

inline void z80_out(uint16_t port, uint8_t data)
{
  if (!(port & 0x01))
  {
    hwopt.BorderColor = (data & 0x07);
    hwopt.SoundBits = (data & 0b00010000);
  }
  else
  {
    // check for AY chip
    if ((port & 0x8002) == 0x8000)
    {
      if (hwopt.hw_model == SPECMDL_128K) {
        if ((port & 0x4000) != 0) {
            AySound::selectRegister(data);
        } else {
            AySound::setRegisterData(data);
        }
      }
    }
    if ((port & 0x8002) == 0)
    {
      // paging
      mem.page(data);
    }
  }
}

  inline void toggleMicLevel() {
    if (micLevel) {
      micLevel = false;
    } else {
      micLevel = true;
    }
  }

  inline void setMicValue(bool value) {
    micLevel = value;
  }

  inline void setMicLow() {
    micLevel = false;
  }

  inline void setMicHigh() {
    micLevel = true;
  }

  bool init_spectrum(int model);
  void reset_spectrum(Z80Regs *);

  bool init_48k();
  bool init_16k();
  bool init_128k(void);
  void reset_128k(void);
};

#endif // #ifdef SPECTRUM_H

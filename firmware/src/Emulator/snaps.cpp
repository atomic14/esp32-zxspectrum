/*=====================================================================
  snaps.c    -> This file includes all the snapshot handling functions
                for the emulator, called from the main loop and when
                the user wants to load/save snapshots files.

  also routines to manage files, i.e. rom files, search in several sites and so on...

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

 Copyright (c) 2000 Santiago Romero Iglesias.
 Email: sromero@escomposlinux.org
 ======================================================================*/
#include <stdint.h>
#include <stdio.h>
#include <algorithm>
#include <string>
#include <string.h>
#include "./z80/z80.h"
#include "./spectrum.h"
#include "./snaps.h"

bool Load(ZXSpectrum *speccy, const char *filename)
{
  // convert the filename to lower case
  std::string filenameStr = filename;
  std::transform(filenameStr.begin(), filenameStr.end(), filenameStr.begin(), ::tolower);
  if (strstr(filenameStr.c_str(), ".sna") != NULL)
  {
    return LoadSNA(speccy, filename);
  }
  else if (strstr(filenameStr.c_str(), ".z80") != NULL)
  {
    return LoadZ80(speccy, filename);
  }
  return false;
}

#define Z80BL_V1UNCOMP 0
#define Z80BL_V1COMPRE 1
#define Z80BL_V2UNCOMP 3
#define Z80BL_V2COMPRE 4

// Decompress a block of data from a V2 or V3 Z80 snapshot
void decompressZ80BlockV2orV3(FILE *fp, uint32_t compressedSize, uint8_t *decompressedData, uint32_t decompressedSize)
{
  uint16_t dataOff = 0;
  uint8_t ed_cnt = 0;
  uint8_t repcnt = 0;
  uint8_t repval = 0;
  uint16_t memidx = 0;

  while (dataOff < compressedSize && memidx < decompressedSize)
  {
    uint8_t databyte = fgetc(fp);
    if (ed_cnt == 0)
    {
      if (databyte != 0xED)
        decompressedData[memidx++] = databyte;
      else
        ed_cnt++;
    }
    else if (ed_cnt == 1)
    {
      if (databyte != 0xED)
      {
        decompressedData[memidx++] = 0xED;
        decompressedData[memidx++] = databyte;
        ed_cnt = 0;
      }
      else
        ed_cnt++;
    }
    else if (ed_cnt == 2)
    {
      repcnt = databyte;
      ed_cnt++;
    }
    else if (ed_cnt == 3)
    {
      repval = databyte;
      for (uint16_t i = 0; i < repcnt; i++)
        decompressedData[memidx++] = repval;
      ed_cnt = 0;
    }
  }
}

// special handling for V1 which is just a stream of RAM - we'll poke this into the Z80 memory
void decompressZ80BlockV1(FILE *fp, uint32_t compressedSize, ZXSpectrum *speccy)
{
  uint32_t inPos = 0; // Position in the compressed data
  // start writing at the start of RAM
  uint32_t outPos = 0x4000;
  uint32_t maxOutPos = 0xC000;

  while (inPos < compressedSize && outPos < maxOutPos)
  {
    // Read the current byte
    uint8_t byte = fgetc(fp);
    inPos++;
    if (inPos < compressedSize)
    {
      uint8_t nextByte = fgetc(fp);
      inPos++;
      // Check if this is an RLE encoded sequence
      if (byte == 0xED && nextByte == 0xED)
      {
        // Read the repetition count and value
        uint8_t repeatCount = fgetc(fp);
        inPos++;
        uint8_t repeatValue = fgetc(fp);
        inPos++;

        // Repeat the value in the output buffer
        for (int i = 0; i < repeatCount; i++)
        {
          if (outPos < maxOutPos)
          {
            speccy->z80_poke(outPos++, repeatValue);
          }
        }
      }
      else
      {
        // If not a compressed sequence, copy the bytes directly
        if (outPos < maxOutPos - 1)
        {
          speccy->z80_poke(outPos++, byte);
          speccy->z80_poke(outPos++, nextByte);
        }
      }
    }
    else
    {
      if (outPos < maxOutPos)
      {
        speccy->z80_poke(outPos++, byte);
      }
    }
  }
}

int getZ80Version(uint8_t *buffer)
{
  if (buffer[6] != 0 || buffer[7] != 0)
  {
    printf("getZ80Version: PC: %x\n", buffer[6] + (buffer[7] << 8));
    return 1;
  }
  int ahb_len = buffer[30] + (buffer[31] << 8);
  printf("AHB: %x\n", ahb_len);
  if (ahb_len == 23)
  {
    return 2;
  }
  if (ahb_len == 54 || ahb_len == 55)
  {
    return 3;
  }
  return 0;
}

void loadZ80Regs(ZXSpectrum *speccy, uint8_t *buffer)
{
  speccy->z80Regs->AF.B.h = buffer[0];
  speccy->z80Regs->AF.B.l = buffer[1];
  speccy->z80Regs->BC.B.l = buffer[2];
  speccy->z80Regs->BC.B.h = buffer[3];
  speccy->z80Regs->HL.B.l = buffer[4];
  speccy->z80Regs->HL.B.h = buffer[5];
  speccy->z80Regs->SP.B.l = buffer[8];
  speccy->z80Regs->SP.B.h = buffer[9];
  speccy->z80Regs->I = buffer[10];
  speccy->z80Regs->R.B.l = buffer[11];
  speccy->z80Regs->R.B.h = 0;
  speccy->hwopt.BorderColor = ((buffer[12] & 0x0E) >> 1);
  speccy->z80Regs->DE.B.l = buffer[13];
  speccy->z80Regs->DE.B.h = buffer[14];
  speccy->z80Regs->BCs.B.l = buffer[15];
  speccy->z80Regs->BCs.B.h = buffer[16];
  speccy->z80Regs->DEs.B.l = buffer[17];
  speccy->z80Regs->DEs.B.h = buffer[18];
  speccy->z80Regs->HLs.B.l = buffer[19];
  speccy->z80Regs->HLs.B.h = buffer[20];
  speccy->z80Regs->AFs.B.h = buffer[21];
  speccy->z80Regs->AFs.B.l = buffer[22];
  speccy->z80Regs->IY.B.l = buffer[23];
  speccy->z80Regs->IY.B.h = buffer[24];
  speccy->z80Regs->IX.B.l = buffer[25];
  speccy->z80Regs->IX.B.h = buffer[26];
  speccy->z80Regs->IFF1 = buffer[27] & 0x01;
  speccy->z80Regs->IFF2 = buffer[28] & 0x01;
  speccy->z80Regs->IM = buffer[29] & 0x03;
}

bool loadZ80Version1(ZXSpectrum *speccy, uint8_t *buffer, FILE *fp)
{
  // only 48K is supported
  if (speccy->hwopt.hw_model != SPECMDL_48K)
  {
    speccy->init_spectrum(SPECMDL_48K);
  }
  // get the file size
  fseek(fp, 0, SEEK_END);
  int totalSize = ftell(fp);
  fseek(fp, 30, SEEK_SET);
  if (buffer[12] & 0x20)
  {
    printf("Loading compressed data %x\n", totalSize - 30);
    decompressZ80BlockV1(fp, totalSize - 30, speccy);
  }
  else
  {
    printf("Loading uncompressed data %x\n", totalSize - 30);
    for (int i = 0; i < 0xC000 && i < totalSize - 30; i++)
    {
      speccy->z80_poke(i + 0x4000, fgetc(fp));
    }
  }
  speccy->z80Regs->PC.B.l = buffer[6];
  speccy->z80Regs->PC.B.h = buffer[7];
  printf("PC: %x\n", speccy->z80Regs->PC.W);
  loadZ80Regs(speccy, buffer);
  printf("PC: %x\n", speccy->z80Regs->PC.W);
  return true;
}

models_enum getHardwareModel(uint8_t *buffer, int version)
{
  int mch = buffer[34];
  if (version == 2)
  {
    if (mch == 0)
      return SPECMDL_48K;
    if (mch == 1)
      return SPECMDL_48K; // + if1
    // if (mch == 2) z80_arch = "SAMRAM";
    if (mch == 3)
      return SPECMDL_128K;
    if (mch == 4)
      return SPECMDL_128K; // + if1
  }
  else if (version == 3)
  {
    if (mch == 0)
      return SPECMDL_48K;
    if (mch == 1)
      return SPECMDL_48K; // + if1
    // if (mch == 2) z80_arch = "SAMRAM";
    if (mch == 3)
      return SPECMDL_128K; // + mgt
    if (mch == 4)
      return SPECMDL_128K;
    if (mch == 5)
      return SPECMDL_128K; // + if1
    if (mch == 6)
      return SPECMDL_128K; // + mgt
    if (mch == 7)
      return SPECMDL_128K; // Spectrum +3
    // if (mch == 9) z80_arch = "Pentagon";
    if (mch == 12)
      return SPECMDL_128K; // Spectrum +2
    if (mch == 13)
      return SPECMDL_128K; // Spectrum +2A
  }
  return SPECMDL_UNKNOWN;
}

bool loadZ80Version2or3(ZXSpectrum *speccy, uint8_t *buffer, int version, FILE *fp)
{
  models_enum hwmodel = getHardwareModel(buffer, version);
  printf("Hardware model %d\n", hwmodel);
  if (speccy->hwopt.hw_model != hwmodel)
  {
    if (!speccy->init_spectrum(hwmodel))
    {
      return false;
    }
  }
  // get the total length of the file
  fseek(fp, 0, SEEK_END);
  int totalSize = ftell(fp);
  // move to the start of the memory pages
  printf("Extra data length %d\n", buffer[30]);
  int dataOffset = 30 + 2 + buffer[30];
  fseek(fp, dataOffset, SEEK_SET);

  MemoryPage *pageMap[16] = {0};
  if (hwmodel == SPECMDL_48K)
  {
    pageMap[4] = speccy->mem.banks[2];
    pageMap[5] = speccy->mem.banks[0];
    pageMap[8] = speccy->mem.banks[5];
  }
  else if (hwmodel == SPECMDL_128K)
  {
    pageMap[3] = speccy->mem.banks[0];
    pageMap[4] = speccy->mem.banks[1];
    pageMap[5] = speccy->mem.banks[2];
    pageMap[6] = speccy->mem.banks[3];
    pageMap[7] = speccy->mem.banks[4];
    pageMap[8] = speccy->mem.banks[5];
    pageMap[9] = speccy->mem.banks[6];
    pageMap[10] = speccy->mem.banks[7];
  }
  while (dataOffset < totalSize)
  {
    int length = fgetc(fp) + (fgetc(fp) << 8);
    int page = fgetc(fp);
    int actualLength = length == 0xFFFF ? 0x4000 : length;
    dataOffset += 3 + actualLength;
    printf("Got page %d, length %d\n", page, length);
    if (pageMap[page] == NULL)
    {
      // nothing to write to this page
      printf("Skipping page %d\n", page);
      continue;
    }
    // if the data is compressed, we need to uncompress it
    if (length == 0xFFFF)
    {
      printf("Reading page %d %d\n", page, actualLength);
      fread(pageMap[page]->data, actualLength, 1, fp);
      pageMap[page]->isDirty = true;
      printf("Read page %d\n", page);
    } else {
      printf("Decompressing page %d\n", page);
      decompressZ80BlockV2orV3(fp, length, pageMap[page]->data, 0x4000);
      pageMap[page]->isDirty = true;
    }
  }
  printf("Setting the PC registers\n");
  speccy->z80Regs->PC.B.l = buffer[32];
  speccy->z80Regs->PC.B.h = buffer[33];
  printf("PC set to %x\n", speccy->z80Regs->PC.W);
  if (hwmodel == SPECMDL_128K)
  {
    printf("Setting paging to %d\n", buffer[35]);
    speccy->mem.page(buffer[35], true);
  }
  else
  {
    // disable paging
    speccy->mem.page(32, true);
  }
  printf("Loding registers\n");
  loadZ80Regs(speccy, buffer);
  return true;
}

bool LoadZ80(ZXSpectrum *speccy, const char *filename)
{
  printf("Loading Z80 file %s\n", filename);
  FILE *fp = fopen(filename, "rb");
  if (!fp)
  {
    printf("Could not open file\n");
    return false;
  }
  // read in the header
  uint8_t buffer[87];
  fread(buffer, 87, 1, fp);
  if (buffer[12] == 255)
    buffer[12] = 1; /*as told in CSS FAQ / .z80 section */
  bool res = false;
  int version = getZ80Version(buffer);
  printf("Z80 version %d\n", version);
  switch (version)
  {
  case 1:
    printf("Loading Z80 version 1\n");
    res = loadZ80Version1(speccy, buffer, fp);
    break;
  case 2:
  case 3:
    printf("Loading Z80 version %d\n", version);
    res = loadZ80Version2or3(speccy, buffer, version, fp);
    break;
  default:
    printf("Unknown Z80 version %d\n", version);
    break;
  }
  fclose(fp);
  return res;
}

/*-----------------------------------------------------------------
 char LoadSNA( Z80Regs *regs, char *filename );
 This loads a .SNA file from disk to the Z80 registers/memory.
------------------------------------------------------------------*/
bool LoadSNA(ZXSpectrum *speccy, const char *filename)
{
  uint8_t page;
  uint8_t buffer[27];
  uint8_t unbyte;
  int model;
  printf("Cargamos un SNA (by filename)\n");
  FILE *fp = fopen(filename, "rb");
  if (!fp)
  {
    printf("Algo fallo cargando el SNA\n");
    return false;
  }
  // get the size of the file
  fseek(fp, 0, SEEK_END);
  int size = ftell(fp);
  if (size == 49179)
    model = SPECMDL_48K; // es un 48Kb
  else if (size == 16411)
    model = SPECMDL_16K; // 16Kb UNSUPPORTED!!
  else
    model = SPECMDL_128K;

  if (speccy->hwopt.hw_model != model)
  {
    speccy->init_spectrum(model);
  }
  fseek(fp, 0, SEEK_SET);
  fread(buffer, 27, 1, fp);
  speccy->z80Regs->I = buffer[0];
  speccy->z80Regs->HLs.B.l = buffer[1];
  speccy->z80Regs->HLs.B.h = buffer[2];
  speccy->z80Regs->DEs.B.l = buffer[3];
  speccy->z80Regs->DEs.B.h = buffer[4];
  speccy->z80Regs->BCs.B.l = buffer[5];
  speccy->z80Regs->BCs.B.h = buffer[6];
  speccy->z80Regs->AFs.B.l = buffer[7];
  speccy->z80Regs->AFs.B.h = buffer[8];
  speccy->z80Regs->HL.B.l = buffer[9];
  speccy->z80Regs->HL.B.h = buffer[10];
  speccy->z80Regs->DE.B.l = buffer[11];
  speccy->z80Regs->DE.B.h = buffer[12];
  speccy->z80Regs->BC.B.l = buffer[13];
  speccy->z80Regs->BC.B.h = buffer[14];
  speccy->z80Regs->IY.B.l = buffer[15];
  speccy->z80Regs->IY.B.h = buffer[16];
  speccy->z80Regs->IX.B.l = buffer[17];
  speccy->z80Regs->IX.B.h = buffer[18];
  speccy->z80Regs->IFF1 = speccy->z80Regs->IFF2 = (buffer[19] & 0x04) >> 2;
  speccy->z80Regs->R.W = buffer[20];
  speccy->z80Regs->AF.B.l = buffer[21];
  speccy->z80Regs->AF.B.h = buffer[22];
  speccy->z80Regs->SP.B.l = buffer[23];
  speccy->z80Regs->SP.B.h = buffer[24];
  speccy->z80Regs->IM = buffer[25];
  speccy->hwopt.BorderColor = buffer[26];

  if (model == SPECMDL_48K)
  {
    // read in each chunk of RAM
    fread(speccy->mem.mappedMemory[1], 0x4000, 1, fp);
    fread(speccy->mem.mappedMemory[2], 0x4000, 1, fp);
    fread(speccy->mem.mappedMemory[3], 0x4000, 1, fp);
    speccy->z80Regs->PC.B.l = speccy->z80_peek(speccy->z80Regs->SP.W);
    speccy->z80Regs->SP.W++;
    speccy->z80Regs->PC.B.h = speccy->z80_peek(speccy->z80Regs->SP.W);
    speccy->z80Regs->SP.W++;
  }
  else if (model == SPECMDL_16K)
  {
    fread(speccy->mem.mappedMemory[1], 0x4000, 1, fp);
    speccy->z80Regs->PC.B.l = speccy->z80_peek(speccy->z80Regs->SP.W);
    speccy->z80Regs->SP.W++;
    speccy->z80Regs->PC.B.h = speccy->z80_peek(speccy->z80Regs->SP.W);
    speccy->z80Regs->SP.W++;
  }
  else
  {
    // 128K
    fseek(fp, 49179, SEEK_SET);
    fread(&unbyte, 1, 1, fp); // FIXME seguro que hay formas mejores
    speccy->z80Regs->PC.B.l = unbyte;
    fread(&unbyte, 1, 1, fp);
    speccy->z80Regs->PC.B.h = unbyte;
    fread(&page, 1, 1, fp);
    // switch the pages to the correct ones
    speccy->mem.page(page, true);
    // ahora empezamos a rellenar ram.
    fseek(fp, 27, SEEK_SET);
    // load up the current RAM
    fread(speccy->mem.mappedMemory[1], 0x4000, 1, fp);
    fread(speccy->mem.mappedMemory[2], 0x4000, 1, fp);
    fread(speccy->mem.mappedMemory[3], 0x4000, 1, fp);
    // load the rest of the pages - ignoring the ones we've already loaded (5, 2, currentPage)
    fseek(fp, 49183, SEEK_SET);
    // this is the page that is mapped into the memory already and has already been loaded
    int currentPage = page & 0x07;
    for (int i = 0; i < 8; i++)
    {
      if (i == currentPage || i == 5 || i == 2)
      {
        // nothing to do for these banks as they've already been loaded
        continue;
      }
      else
      {
        fread(speccy->mem.banks[i]->data, 0x4000, 1, fp);
      }
    }
  }
  fclose(fp);
  return true;
}

uint8_t LoadSCR(ZXSpectrum *speccy, FILE *fp)
{
  int i;
  // FIX use fwrite
  // FIX2 must be the actual video page
  for (i = 0; i < 6912; i++)
    speccy->z80_poke(16384 + i, fgetc(fp));
  return 0;
}

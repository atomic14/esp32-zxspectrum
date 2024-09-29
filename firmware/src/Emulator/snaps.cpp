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
#include <Arduino.h>
#include <stdint.h>
#include <stdio.h>
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

// void writeZ80block(ZXSpectrum *speccy, int block, int offset, FILE *fp);

// uint8_t SaveScreenshot(ZXSpectrum *speccy, const char *fname)
// {
//   FILE *snafp = fopen(fname, "wb");
//   if (snafp)
//   {
//     SaveSCR(speccy, snafp);
//     fclose(snafp);
//     return (1);
//   }
//   return (0);
// }

/*-----------------------------------------------------------------
 char LoadZ80( Z80Regs *regs, FILE *fp );
 This loads a .Z80 file from disk to the Z80 registers/memory.

 void UncompressZ80 (int tipo, int pc, Z80Regs *regs, FILE *fp)
 This load and uncompres a Z80 block to pc adress memory.

 The Z80 Load Routine is (C) 2001 Alvaro Alea Fdz.
 e-mail: ALEAsoft@yahoo.com  Distributed under GPL2
------------------------------------------------------------------*/
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
    return 1;
  }
  int ahb_len = buffer[30] + (buffer[31] << 8);
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
    Serial.printf("Loading compressed data %x\n", totalSize - 30);
    decompressZ80BlockV1(fp, totalSize - 30, speccy);
  }
  else
  {
    Serial.printf("Loading uncompressed data %x\n", totalSize - 30);
    for (int i = 0; i < 0xC000 && i < totalSize - 30; i++)
    {
      speccy->z80_poke(i + 0x4000, fgetc(fp));
    }
  }
  speccy->z80Regs->PC.B.l = buffer[6];
  speccy->z80Regs->PC.B.h = buffer[7];
  Serial.printf("PC: %x\n", speccy->z80Regs->PC.W);
  loadZ80Regs(speccy, buffer);
  Serial.printf("PC: %x\n", speccy->z80Regs->PC.W);
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
      return SPECMDL_48K; // + mgt
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
  Serial.printf("Hardware model %d\n", hwmodel);
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
  Serial.printf("Extra data length %d\n", buffer[30]);
  int dataOffset = 30 + 2 + buffer[30];
  fseek(fp, dataOffset, SEEK_SET);

  uint8_t *pageMap[16] = {0};
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
    dataOffset += 3 + length;
    Serial.printf("Got page %d, length %d\n", page, length);
    if (pageMap[page] == NULL)
    {
      // nothing to write to this page
      Serial.printf("Skipping page %d\n", page);
      continue;
    }
    // if the data is compressed, we need to uncompress it
    if (length == 0xFFFF)
    {
      length = 0x4000;
      Serial.printf("Reading page %d\n", page);
      fread(pageMap[page], length, 1, fp);
    } else {
      Serial.printf("Decompressing page %d\n", page);
      decompressZ80BlockV2orV3(fp, length, pageMap[page], 0x4000);
    }
  }
  speccy->z80Regs->PC.B.l = buffer[32];
  speccy->z80Regs->PC.B.h = buffer[33];
  Serial.printf("PC set to %x\n", speccy->z80Regs->PC.W);
  if ((hwmodel == SPECMDL_128K))
  {
    speccy->mem.page(buffer[35], true);
  }
  else
  {
    // disable paging
    speccy->mem.page(32, true);
  }
  loadZ80Regs(speccy, buffer);
  return true;
}

bool LoadZ80(ZXSpectrum *speccy, const char *filename)
{
  Serial.printf("Loading Z80 file %s\n", filename);
  FILE *fp = fopen(filename, "rb");
  if (!fp)
  {
    Serial.println("Algo fallo cargando el SNA\n");
    return false;
  }
  // read in the header
  uint8_t buffer[87];
  fread(buffer, 87, 1, fp);
  // dump out the buffer
  for (int i = 0; i < 30; i++)
  {
    Serial.printf("%d:%02X\n", i, buffer[i]);
  }
  if (buffer[12] == 255)
    buffer[12] = 1; /*as told in CSS FAQ / .z80 section */
  bool res = false;
  int version = getZ80Version(buffer);
  switch (version)
  {
  case 1:
    Serial.printf("Loading Z80 version 1\n");
    res = loadZ80Version1(speccy, buffer, fp);
    break;
  case 2:
  case 3:
    Serial.printf("Loading Z80 version %d\n", version);
    res = loadZ80Version2or3(speccy, buffer, version, fp);
    break;
  default:
    Serial.printf("Unknown Z80 version %d\n", version);
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
  Serial.println("Cargamos un SNA (by filename)\n");
  FILE *fp = fopen(filename, "rb");
  if (!fp)
  {
    Serial.println("Algo fallo cargando el SNA\n");
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

  if (model = SPECMDL_48K)
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
  else if (model = SPECMDL_16K)
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
        fread(speccy->mem.banks[i], 0x4000, 1, fp);
      }
    }
  }
  fclose(fp);
  return true;
}

/*-----------------------------------------------------------------
 char SaveSNA( Z80Regs *regs, FILE *fp );
 This saves a .SNA file from the Z80 registers/memory to disk.
------------------------------------------------------------------*/
// uint8_t SaveSNA(ZXSpectrum *speccy, FILE *fp)
// {
//   unsigned char sptmpl, sptmph;
//   //  int c;

//   // SNA solo esta soportado en 48K, 128K y +2

//   if ((speccy->hwopt.hw_model != SPECMDL_48K) && (speccy->hwopt.hw_model != SPECMDL_128K))
//   {
//     Serial.println("El modelo de Spectrum utilizado");
//     Serial.println("No permite grabar el snapshot en formato SNA");
//     Serial.println("Utilize otro tipo de archivo (extension)");
//     return 1;
//   }

//   /* save the .SNA header */
//   fputc(speccy->z80Regs->I, fp);
//   fputc(speccy->z80Regs->HLs.B.l, fp);
//   fputc(speccy->z80Regs->HLs.B.h, fp);
//   fputc(speccy->z80Regs->DEs.B.l, fp);
//   fputc(speccy->z80Regs->DEs.B.h, fp);
//   fputc(speccy->z80Regs->BCs.B.l, fp);
//   fputc(speccy->z80Regs->BCs.B.h, fp);
//   fputc(speccy->z80Regs->AFs.B.l, fp);
//   fputc(speccy->z80Regs->AFs.B.h, fp);
//   fputc(speccy->z80Regs->HL.B.l, fp);
//   fputc(speccy->z80Regs->HL.B.h, fp);
//   fputc(speccy->z80Regs->DE.B.l, fp);
//   fputc(speccy->z80Regs->DE.B.h, fp);
//   fputc(speccy->z80Regs->BC.B.l, fp);
//   fputc(speccy->z80Regs->BC.B.h, fp);
//   fputc(speccy->z80Regs->IY.B.l, fp);
//   fputc(speccy->z80Regs->IY.B.h, fp);
//   fputc(speccy->z80Regs->IX.B.l, fp);
//   fputc(speccy->z80Regs->IX.B.h, fp);
//   fputc(speccy->z80Regs->IFF1 << 2, fp);
//   fputc(speccy->z80Regs->R.W & 0xFF, fp);
//   fputc(speccy->z80Regs->AF.B.l, fp);
//   fputc(speccy->z80Regs->AF.B.h, fp);

//   //  sptmpl = Z80MemRead (speccy->z80Regs->SP.W - 1, speccy->z80Regs);
//   //  sptmph = Z80MemRead (speccy->z80Regs->SP.W - 2, speccy->z80Regs);
//   sptmpl = speccy->z80_peek(speccy->z80Regs->SP.W - 1);
//   sptmph = speccy->z80_peek(speccy->z80Regs->SP.W - 2);

//   if (speccy->hwopt.hw_model == SPECMDL_48K)
//   { // code for the 48K version.
//     Serial.printf("Guardando SNA 48K\n");
//     /* save PC on the stack */
//     //    Z80MemWrite (--(speccy->z80Regs->SP.W), speccy->z80Regs->PC.B.h, speccy->z80Regs);
//     //    Z80MemWrite (--(speccy->z80Regs->SP.W), speccy->z80Regs->PC.B.l, speccy->z80Regs);
//     speccy->z80_poke(--(speccy->z80Regs->SP.W), speccy->z80Regs->PC.B.h);
//     speccy->z80_poke(--(speccy->z80Regs->SP.W), speccy->z80Regs->PC.B.l);

//     fputc(speccy->z80Regs->SP.B.l, fp);
//     fputc(speccy->z80Regs->SP.B.h, fp);
//     fputc(speccy->z80Regs->IM, fp);
//     fputc(speccy->hwopt.BorderColor, fp);

//     /* save the RAM contents */
//     //  fwrite (speccy->z80Regs->RAM + 16384, 48 * 1024, 1, fp);
//     fwrite(speccy->mem.p + 16384, 0x4000 * 3, 1, fp);

//     /* restore the stack and the SP value */
//     speccy->z80Regs->SP.W += 2;
//     //    Z80MemWrite (speccy->z80Regs->SP.W - 1, sptmpl, speccy->z80Regs);
//     //    Z80MemWrite (speccy->z80Regs->SP.W - 2, sptmph, speccy->z80Regs);
//     speccy->z80_poke(speccy->z80Regs->SP.W - 1, sptmpl);
//     speccy->z80_poke(speccy->z80Regs->SP.W - 2, sptmph);
//   }
//   else
//   { // code for the 128K version
//     Serial.printf("Guardando SNA 128K\n");
//     fputc(speccy->z80Regs->SP.B.l, fp);
//     fputc(speccy->z80Regs->SP.B.h, fp);
//     fputc(speccy->z80Regs->IM, fp);
//     fputc(speccy->hwopt.BorderColor, fp);

//     // volcar la ram
//     //  Volcar 5
//     //  for (c=mem.ro[1];(c<mem.ro[1]+0x4000);c++) fputc (speccy->mem.p[c],fp);
//     fwrite(speccy->mem.p + speccy->mem.ro[1], 0x4000, 1, fp);

//     // Volcar 2
//     //  for (c=mem.ro[2];(c<mem.ro[2]+0x4000);c++) fputc (speccy->mem.p[c],fp);
//     fwrite(speccy->mem.p + speccy->mem.ro[2], 0x4000, 1, fp);

//     // volcar N
//     //  for (c=mem.ro[3];(c<mem.ro[3]+0x4000);c++) fputc (speccy->mem.p[c],fp);
//     fwrite(speccy->mem.p + speccy->mem.ro[3], 0x4000, 1, fp);

//     // resto de cosas
//     fputc(speccy->z80Regs->PC.B.l, fp);
//     fputc(speccy->z80Regs->PC.B.h, fp);
//     fputc(speccy->hwopt.BANKM, fp);
//     fputc(0, fp); // TR-DOS rom paged (1) or not (0)

//     // volcar el resto de ram.

//     //  Volcar 0 si no en (4)
//     if (speccy->mem.ro[3] != (0 * speccy->mem.sp))
//       //    for (c=0*mem.sp;c<((0*mem.sp)+0x4000);c++) fputc (speccy->mem.p[c],fp);
//       fwrite(speccy->mem.p + (0 * speccy->mem.sp), 0x4000, 1, fp);

//     //  Volcar 1 si no en (4)
//     if (speccy->mem.ro[3] != (1 * speccy->mem.sp))
//       //    for (c=1*mem.sp;c<((1*mem.sp)+0x4000);c++) fputc (speccy->mem.p[c],fp);
//       fwrite(speccy->mem.p + (1 * speccy->mem.sp), 0x4000, 1, fp);

//     //  Volcar 3 si no en (4)
//     if (speccy->mem.ro[3] != (3 * speccy->mem.sp))
//       //    for (c=3*mem.sp;c<((3*mem.sp)+0x4000);c++) fputc (speccy->mem.p[c],fp);
//       fwrite(speccy->mem.p + (3 * speccy->mem.sp), 0x4000, 1, fp);

//     //  Volcar 4 si no en (4)
//     if (speccy->mem.ro[3] != (4 * speccy->mem.sp))
//       //    for (c=4*mem.sp;c<((4*mem.sp)+0x4000);c++) fputc (speccy->mem.p[c],fp);
//       fwrite(speccy->mem.p + (4 * speccy->mem.sp), 0x4000, 1, fp);

//     //  Volcar 6 si no en (4)
//     if (speccy->mem.ro[3] != (6 * speccy->mem.sp))
//       //    for (c=6*mem.sp;c<((6*mem.sp)+0x4000);c++) fputc (speccy->mem.p[c],fp);
//       fwrite(speccy->mem.p + (6 * speccy->mem.sp), 0x4000, 1, fp);

//     //  Volcar 7 si no en (4)
//     if (speccy->mem.ro[3] != (7 * speccy->mem.sp))
//       //    for (c=7*mem.sp;c<((7*mem.sp)+0x4000);c++) fputc (speccy->mem.p[c],fp);
//       fwrite(speccy->mem.p + (7 * speccy->mem.sp), 0x4000, 1, fp);
//   }
//   return (0);
// }

// uint8_t SaveSP(ZXSpectrum *speccy, FILE *fp)
// {
//   // save the .SP header
//   fputc('S', fp);
//   fputc('P', fp);
//   fputc(00, fp);
//   fputc(0xC0, fp); // 49152
//   fputc(00, fp);
//   fputc(0x40, fp); // 16384
//   // save the state
//   fputc(speccy->z80Regs->BC.B.l, fp);
//   fputc(speccy->z80Regs->BC.B.h, fp);
//   fputc(speccy->z80Regs->DE.B.l, fp);
//   fputc(speccy->z80Regs->DE.B.h, fp);
//   fputc(speccy->z80Regs->HL.B.l, fp);
//   fputc(speccy->z80Regs->HL.B.h, fp);
//   fputc(speccy->z80Regs->AF.B.l, fp);
//   fputc(speccy->z80Regs->AF.B.h, fp);
//   fputc(speccy->z80Regs->IX.B.l, fp);
//   fputc(speccy->z80Regs->IX.B.h, fp);
//   fputc(speccy->z80Regs->IY.B.l, fp);
//   fputc(speccy->z80Regs->IY.B.h, fp);
//   fputc(speccy->z80Regs->BCs.B.l, fp);
//   fputc(speccy->z80Regs->BCs.B.h, fp);
//   fputc(speccy->z80Regs->DEs.B.l, fp);
//   fputc(speccy->z80Regs->DEs.B.h, fp);
//   fputc(speccy->z80Regs->HLs.B.l, fp);
//   fputc(speccy->z80Regs->HLs.B.h, fp);
//   fputc(speccy->z80Regs->AFs.B.l, fp);
//   fputc(speccy->z80Regs->AFs.B.h, fp);
//   fputc(speccy->z80Regs->R.B.l, fp);
//   fputc(speccy->z80Regs->I, fp);
//   fputc(speccy->z80Regs->SP.B.l, fp);
//   fputc(speccy->z80Regs->SP.B.h, fp);
//   fputc(speccy->z80Regs->PC.B.l, fp);
//   fputc(speccy->z80Regs->PC.B.h, fp);
//   fputc(0, fp);
//   fputc(0, fp);
//   fputc(speccy->hwopt.BorderColor, fp);
//   fputc(0, fp);
//   // Estado, pendiente por poner:  Si hay una int pendiente (bit 4), valor de flash (bit 5).
//   fputc(0, fp);
//   fputc((((speccy->z80Regs->IFF2) << 2) + ((speccy->z80Regs->IM) == 2 ? 0x02 : 0x00) + speccy->z80Regs->IFF1),
//         fp);
//   // Save the ram
//   //  fwrite (speccy->z80Regs->RAM + 16384, 48 * 1024, 1, fp);
//   fwrite(speccy->mem.p + 16384, 0x4000 * 3, 1, fp);

//   return (0);
// }

// void writeZ80block(ZXSpectrum *speccy, int block, int offset, FILE *fp)
// {
//   extern tipo_mem mem;
//   fputc(0xff, fp);
//   fputc(0xff, fp);
//   fputc((byte)block, fp);
//   fwrite(speccy->mem.p + offset, 0x4000, 1, fp);
// }

// uint8_t SaveZ80(ZXSpectrum *speccy, FILE *fp)
// {
//   int c;

//   // 48K  .z80 are saved as ver 1.45
//   Serial.printf("Guardando Z80...\n");
//   fputc(speccy->z80Regs->AF.B.h, fp);
//   fputc(speccy->z80Regs->AF.B.l, fp);
//   fputc(speccy->z80Regs->BC.B.l, fp);
//   fputc(speccy->z80Regs->BC.B.h, fp);
//   fputc(speccy->z80Regs->HL.B.l, fp);
//   fputc(speccy->z80Regs->HL.B.h, fp);

//   if (speccy->hwopt.hw_model == SPECMDL_48K)
//   { // si no es 48K entonces V 3.0
//     Serial.printf("...de 48K\n");
//     fputc(speccy->z80Regs->PC.B.l, fp);
//     fputc(speccy->z80Regs->PC.B.h, fp);
//   }
//   else
//   {
//     Serial.printf("...de NO 48K\n");
//     fputc(0, fp);
//     fputc(0, fp);
//   }

//   fputc(speccy->z80Regs->SP.B.l, fp);
//   fputc(speccy->z80Regs->SP.B.h, fp);
//   fputc(speccy->z80Regs->I, fp);
//   fputc((speccy->z80Regs->R.B.l & 0x7F), fp);
//   fputc((((speccy->z80Regs->R.B.l & 0x80) >> 7) | ((speccy->hwopt.BorderColor & 0x07) << 1)),
//         fp); // Datos sin comprimir por defecto.
//   fputc(speccy->z80Regs->DE.B.l, fp);
//   fputc(speccy->z80Regs->DE.B.h, fp);
//   fputc(speccy->z80Regs->BCs.B.l, fp);
//   fputc(speccy->z80Regs->BCs.B.h, fp);
//   fputc(speccy->z80Regs->DEs.B.l, fp);
//   fputc(speccy->z80Regs->DEs.B.h, fp);
//   fputc(speccy->z80Regs->HLs.B.l, fp);
//   fputc(speccy->z80Regs->HLs.B.h, fp);
//   fputc(speccy->z80Regs->AFs.B.h, fp);
//   fputc(speccy->z80Regs->AFs.B.l, fp);
//   fputc(speccy->z80Regs->IY.B.l, fp);
//   fputc(speccy->z80Regs->IY.B.h, fp);
//   fputc(speccy->z80Regs->IX.B.l, fp);
//   fputc(speccy->z80Regs->IX.B.h, fp);
//   fputc(speccy->z80Regs->IFF1, fp);
//   fputc(speccy->z80Regs->IFF2, fp);
//   fputc((speccy->z80Regs->IM & 0x7), fp); // issue 2 and joystick cfg not saved.

//   if (speccy->hwopt.hw_model == SPECMDL_48K)
//   {
//     //  fwrite (speccy->z80Regs->RAM + 16384, 48 * 1024, 1, fp);
//     fwrite(speccy->mem.p + 16384, 0x4000 * 3, 1, fp);
//   }
//   else
//   { // aki viene la parte de los 128K y demas
//     fputc((speccy->hwopt.hw_model == SPECMDL_PLUS3 ? 55 : 54), fp);
//     fputc(0, fp);
//     fputc(speccy->z80Regs->PC.B.l, fp);
//     fputc(speccy->z80Regs->PC.B.h, fp);
//     switch (speccy->hwopt.hw_model)
//     {
//     case SPECMDL_16K:
//       fputc(0, fp);
//       fputc(0, fp);
//       break;
//     case SPECMDL_INVES:
//       fputc(64, fp);
//       fputc(0, fp);
//       break;
//     case SPECMDL_128K:
//       fputc(4, fp);
//       fputc(speccy->hwopt.BANKM, fp);
//       break;
//     case SPECMDL_PLUS2:
//       fputc(12, fp);
//       fputc(speccy->hwopt.BANKM, fp);
//       break;
//     case SPECMDL_PLUS3: // as there is no disk emuation, alwais save as +2A
//       fputc(13, fp);
//       fputc(speccy->hwopt.BANKM, fp);
//     default:
//       Serial.printf("ERROR: HW Type Desconocido, nunca deberias ver esto.\n");
//       fputc(0, fp);
//       fputc(0, fp);
//       break;
//     }
//     fputc(0, fp); // if1 not paged.
//     if (speccy->hwopt.hw_model == SPECMDL_16K)
//       fputc(0x81, fp);
//     else
//       fputc(1, fp); // siempre emulamos el registro R (al menos que yo sepa)

//     for (c = 0; c < (1 + 16 + 3 + 1 + 4 + 20 + 3); c++)
//       fputc(0, fp);

//     if (speccy->hwopt.hw_model == SPECMDL_PLUS3)
//       fputc(speccy->hwopt.BANK678, fp);

//     switch (speccy->hwopt.hw_model)
//     {
//     case SPECMDL_16K:
//       writeZ80block(speccy, 8, 0x4000, fp);
//       break;
//     case SPECMDL_INVES:
//       writeZ80block(speccy, 8, 0x4000, fp);
//       writeZ80block(speccy, 4, 0x8000, fp);
//       writeZ80block(speccy, 5, 0xC000, fp);
//       writeZ80block(speccy, 0, 0x10000, fp);
//       break;
//     case SPECMDL_128K:
//     case SPECMDL_PLUS2:
//       writeZ80block(speccy, 3, 0x0000, fp);
//       writeZ80block(speccy, 4, 0x4000, fp);
//       writeZ80block(speccy, 5, 0x8000, fp);
//       writeZ80block(speccy, 6, 0xC000, fp);
//       writeZ80block(speccy, 7, 0x10000, fp);
//       writeZ80block(speccy, 8, 0x14000, fp);
//       writeZ80block(speccy, 9, 0x18000, fp);
//       writeZ80block(speccy, 10, 0x1C000, fp);
//       break;
//     }
//   }
//   return (0);
// }

uint8_t LoadSCR(ZXSpectrum *speccy, FILE *fp)
{
  int i;
  // FIX use fwrite
  // FIX2 must be the actual video page
  for (i = 0; i < 6912; i++)
    speccy->z80_poke(16384 + i, fgetc(fp));
  return 0;
}

/*-----------------------------------------------------------------
 char SaveSCR( Z80Regs *regs, FILE *fp );
 This saves a .SCR file from the Spectrum videomemory.
------------------------------------------------------------------*/
// uint8_t SaveSCR(ZXSpectrum *speccy, FILE *fp)
// {
//   int i;
//   /* Save the contents of VideoRAM to file: write the 6192 bytes
//    * starting on the memory address 16384 */
//   // FIXME: user  fwrite in the actual videopage
//   for (i = 0; i < 6912; i++)
//     fputc(speccy->readvmem(i, speccy->hwopt.videopage), fp);
//   //    fputc (Z80MemRead (16384 + i, regs), fp);

//   return (0);
// }

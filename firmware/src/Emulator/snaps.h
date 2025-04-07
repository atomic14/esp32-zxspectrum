/*=====================================================================
  snaps.h    -> Header file for snaps.c.

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
#ifndef SNAPS_H
#define SNAPS_H

#ifndef DOSSEP
#define DOSSEP '/'
#endif
#define TRUE 1
#define FALSE 0
#include "./z80/z80.h"
#include "./spectrum.h"

bool Load( ZXSpectrum *speccy, const char *filename);
bool LoadSNA(ZXSpectrum *speccy, const char *filename);
bool LoadZ80(ZXSpectrum *speccy, const char *filename);
class Z80Writer
{
protected:
  ZXSpectrum *speccy;
  virtual bool start() = 0;
  virtual void writeByte(uint8_t byte) = 0;
  virtual void writeBytes(const uint8_t *data, size_t size) = 0;
  virtual void end() = 0;
public:
  Z80Writer(ZXSpectrum *speccy) : speccy(speccy) {}
  virtual ~Z80Writer() {}
  virtual bool saveZ80()
  {
    if (!start())
    {
      return false;
    }  
    // Version 3 header has 30 + 54 + 2 bytes
    uint8_t header[30 + 54 + 2] = {0};
  
    // Fill the first 30 bytes of the header with the Z80 register data
    header[0] = speccy->z80Regs->AF.B.h;
    header[1] = speccy->z80Regs->AF.B.l;
    header[2] = speccy->z80Regs->BC.B.l;
    header[3] = speccy->z80Regs->BC.B.h;
    header[4] = speccy->z80Regs->HL.B.l;
    header[5] = speccy->z80Regs->HL.B.h;
    header[6] = 0; // PC is stored separately in the header
    header[7] = 0;
    header[8] = speccy->z80Regs->SP.B.l;
    header[9] = speccy->z80Regs->SP.B.h;
    header[10] = speccy->z80Regs->I;
    header[11] = speccy->z80Regs->R.B.l;
    header[12] = (speccy->hwopt.BorderColor << 1) & 0x0E;
    header[13] = speccy->z80Regs->DE.B.l;
    header[14] = speccy->z80Regs->DE.B.h;
    header[15] = speccy->z80Regs->BCs.B.l;
    header[16] = speccy->z80Regs->BCs.B.h;
    header[17] = speccy->z80Regs->DEs.B.l;
    header[18] = speccy->z80Regs->DEs.B.h;
    header[19] = speccy->z80Regs->HLs.B.l;
    header[20] = speccy->z80Regs->HLs.B.h;
    header[21] = speccy->z80Regs->AFs.B.h;
    header[22] = speccy->z80Regs->AFs.B.l;
    header[23] = speccy->z80Regs->IY.B.l;
    header[24] = speccy->z80Regs->IY.B.h;
    header[25] = speccy->z80Regs->IX.B.l;
    header[26] = speccy->z80Regs->IX.B.h;
    header[27] = speccy->z80Regs->IFF1;
    header[28] = speccy->z80Regs->IFF2;
    header[29] = speccy->z80Regs->IM & 0x03;
  
    // Version 3 header (starting from byte 30)
    header[30] = 54; // Number of additional bytes for version 3 header
    header[32] = speccy->z80Regs->PC.B.l;
    header[33] = speccy->z80Regs->PC.B.h;
  
    // Hardware model (48K or 128K)
    if (speccy->hwopt.hw_model == SPECMDL_48K)
    {
      header[34] = 0;
    }
    else if (speccy->hwopt.hw_model == SPECMDL_128K)
    {
      header[34] = 3;
      header[35] = speccy->mem.hwBank; // Current memory page
    }
  
    // Fill the rest of the version 3 header fields as necessary
    // e.g., include additional data like the interrupt mode or border color as needed.
    writeBytes(header, sizeof(header));
  
    // Write the memory pages, uncompressed
    uint8_t *pageMap[16] = {0};
  
    // Map pages based on 48K or 128K model
    if (speccy->hwopt.hw_model == SPECMDL_48K)
    {
      pageMap[4] = speccy->mem.banks[2]->data;
      pageMap[5] = speccy->mem.banks[0]->data;
      pageMap[8] = speccy->mem.banks[5]->data;
    }
    else if (speccy->hwopt.hw_model == SPECMDL_128K)
    {
      pageMap[3] = speccy->mem.banks[0]->data;
      pageMap[4] = speccy->mem.banks[1]->data;
      pageMap[5] = speccy->mem.banks[2]->data;
      pageMap[6] = speccy->mem.banks[3]->data;
      pageMap[7] = speccy->mem.banks[4]->data;
      pageMap[8] = speccy->mem.banks[5]->data;
      pageMap[9] = speccy->mem.banks[6]->data;
      pageMap[10] = speccy->mem.banks[7]->data;
    }
  
    for (int page = 0; page < 16; ++page)
    {
        if (pageMap[page] != NULL)
        {
            printf("Writing page: %d\n", page);
            // Write uncompressed page
            uint16_t length = 0xFFFF; // Indicates uncompressed block
            writeByte(length & 0xFF); // low byte
            writeByte(length >> 8);   // high byte
            writeByte(page);          // page number
            writeBytes(pageMap[page], 0x4000); // Write full 16KB page
        }
    }
    end();
    return true;
  }
};


class Z80FileWriter : public Z80Writer
{
  protected:
    FILE *fp;
    std::string filename;
    virtual bool start() {
      fp = fopen(filename.c_str(), "wb");
      if (!fp)
      {
        printf("Error opening file for writing: %s\n", filename.c_str());
        return false;
      }
      return true;
    }
    virtual void writeByte(uint8_t byte) {
      if (fp)
      {
        fputc(byte, fp);
      }
    }
    virtual void writeBytes(const uint8_t *data, size_t size) {
      if (fp)
      {
        fwrite(data, 1, size, fp);
      }
    }
    virtual void end() {
      if (fp)
      {
        fclose(fp);
        fp = NULL;
      }
    }
  public:
    Z80FileWriter(ZXSpectrum *speccy, const char *filename) : Z80Writer(speccy), filename(filename) {}
};

class Z80MemoryWriter : public Z80Writer
{
  protected:
    uint8_t *buffer = nullptr;
    size_t bufferSize = 0;
    size_t offset = 0;
    virtual bool start() {
      offset = 0;
      return true;
    }
    virtual void writeByte(uint8_t byte) {
      if (offset < bufferSize)
      {
        buffer[offset++] = byte;
      }
    }
    virtual void writeBytes(const uint8_t *data, size_t size) {
      if (offset + size <= bufferSize)
      {
        memcpy(buffer + offset, data, size);
        offset += size;
      }
    }
    virtual void end() {}
  public:
    Z80MemoryWriter(ZXSpectrum *speccy): Z80Writer(speccy) {
      // create space for the z80 file - the header is 30 + 54 + 2 bytes and 
      // if we are a 48K machine we write 3 pages of 16384 + 3 bytes
      // and if we are a 128K machine we write 8 pages of 16384 + 3 bytes
      if (speccy->hwopt.hw_model == SPECMDL_48K) {
        bufferSize = 30 + 54 + 2 + 3 * (16384 + 3);
      }
      if (speccy->hwopt.hw_model == SPECMDL_128K) {
        bufferSize = 30 + 54 + 2 + 8 * (16384 + 3);
      }
      buffer = (uint8_t *)malloc(bufferSize);
    }
    ~Z80MemoryWriter() {
      free(buffer);
    }
    size_t size() {
      return bufferSize;
    }
    uint8_t *data() {
      return buffer;
    }
};

#endif  // #ifdef SNAPS_H

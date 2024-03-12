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
#include "z80.h"
#include "spectrum.h"

enum tipos_archivo { TYPE_NULL=0, TYPE_Z80, TYPE_SNA,
			TYPE_SP, TYPE_SCR }; 
int typeoffile(const char *);

uint8_t LoadSnapshot (ZXSpectrum *speccy, const char *filename, tipo_mem &mem);
uint8_t LoadSP  (ZXSpectrum *speccy, FILE *, tipo_mem &mem);
//uint8_t LoadSNA (ZXSpectrum *speccy, FILE *);
uint8_t LoadZ80 (ZXSpectrum *speccy, FILE *);
uint8_t LoadSCR (ZXSpectrum *speccy, FILE *);

int Load_SNA (ZXSpectrum *speccy, const char *filename);
int Load_SCR (ZXSpectrum *speccy, const char *filename);

uint8_t SaveSnapshot   (ZXSpectrum *speccy, const char *filename);
uint8_t SaveScreenshot (ZXSpectrum *speccy, const char *filename);
uint8_t SaveSP  (ZXSpectrum *speccy, FILE *);
uint8_t SaveSNA (ZXSpectrum *speccy, FILE *);
uint8_t SaveZ80 (ZXSpectrum *speccy, FILE *);
uint8_t SaveSCR (ZXSpectrum *speccy, FILE *);

#endif  // #ifdef SNAPS_H

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
#ifdef __cplusplus
  extern "C" {
#endif  
#ifndef DOSSEP
#define DOSSEP '/'
#endif
#define TRUE 1
#define FALSE 0
#include "z80.h"
#include "spectrum.h"

enum tipos_archivo { TYPE_NULL=0, TYPE_TZX ,TYPE_TAP, TYPE_Z80, TYPE_SNA,
			TYPE_SP, TYPE_SCR }; 
int typeoffile(const char *);


uint8_t LoadSnapshot (Z80Regs *, const char *filename, tipo_mem &mem);
uint8_t LoadSP  (Z80Regs *, FILE *, tipo_mem &mem);
//uint8_t LoadSNA (Z80Regs *, FILE *);
uint8_t LoadZ80 (Z80Regs *, FILE *);
uint8_t LoadSCR (Z80Regs *, FILE *);

int Load_SNA (Z80Regs *, const char *filename);
int Load_SCR (Z80Regs *, const char *filename);

uint8_t SaveSnapshot   (Z80Regs * regs, const char *filename);
uint8_t SaveScreenshot (Z80Regs * regs, const char *filename);
uint8_t SaveSP  (Z80Regs *, FILE *);
uint8_t SaveSNA (Z80Regs *, FILE *);
uint8_t SaveZ80 (Z80Regs *, FILE *);
uint8_t SaveSCR (Z80Regs *, FILE *);



/* RUTINAS DE CINTA: GENERICAS */
FILE *InitTape(FILE *fp);
uint8_t LoadTAP (Z80Regs *, FILE *);
uint8_t RewindTAP (Z80Regs *, FILE *);

/* RUTINAS DE CINTA: ESPECIFICAS */
uint8_t TAP_init(FILE *fp);
uint8_t TAP_loadblock(Z80Regs * regs, FILE * fp);
uint8_t TAP_rewind(FILE *fp);

uint8_t TZX_init(FILE *fp);
uint8_t TZX_loadblock(Z80Regs * regs, FILE * fp);
uint8_t TZX_rewind();
uint8_t TZX_genindex(FILE *fp);


#ifdef __cplusplus
  }
#endif
#endif  // #ifdef SNAPS_H

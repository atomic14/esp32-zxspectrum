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
#include <string.h>
#include <SPIFFS.h>
#include "z80.h"
#include "spectrum.h"
#include "snaps.h"
#include "hardware.h"

extern FILE *tapfile;

// Generic routines based on the filename extension:
uint8_t LoadSnapshot (Z80Regs * regs, const char *fname, tipo_mem &mem) {
  FILE *snafp;
  File snapfile;
  uint8_t ret;
  AS_printf("Cargamos un snapshot (por definir)\n");
  snapfile = SPIFFS.open(fname, "rb");
  if (!snapfile) {
    AS_printf("  Fallo el intento de abrir\n");
    return (0);
  }
  switch (typeoffile (fname)) {
  case TYPE_SP:
    LoadSP (regs, snafp, mem);
    snapfile.close();
    return (1);
    break;
  case TYPE_SNA:
//    LoadSNA (regs, snafp);
    Load_SNA (regs, fname);
    snapfile.close();
    return (1);
    break;
  case TYPE_Z80:
    LoadZ80 (regs, snafp);
    snapfile.close();
    return (1);
    break;
  case TYPE_SCR:
    LoadSCR (regs, snafp);
    snapfile.close();
    return (1);
    break;
  default:
    AS_printf("  tipo desconocido\n");
    snapfile.close();
    return (0);
    break;
  }
}


uint8_t SaveSnapshot (Z80Regs * regs, const char *fname)
{
  File snapfile;
  FILE *snafp;
//  char ret;
  snapfile = SPIFFS.open(fname, "wb");
  if (!snapfile) return (0);

  switch (typeoffile (fname)) {
  case TYPE_SP:
    SaveSP (regs, snafp);
    snapfile.close();
    return (1);
    break;
  case TYPE_SNA:
    SaveSNA (regs, snafp);
    snapfile.close();
    return (1);
    break;
  case TYPE_SCR:		// deberiamos quitar la opcion de guardar .scr del menu ??
    SaveSCR (regs, snafp);
    snapfile.close();
    return (1);
    break;
  case TYPE_Z80:
    SaveZ80 (regs, snafp);
    snapfile.close();
    return (1);
    break;
  default:
    snapfile.close();
    return (0);
    break;
  }
  snapfile.close();
  return (0);
}


uint8_t SaveScreenshot (Z80Regs * regs, const char *fname)
{
  File snapfile;
  FILE *snafp;
  snapfile = SPIFFS.open(fname, "wb");
  if (snapfile) {
    SaveSCR (regs, snafp);
//    fclose (snafp);
    snapfile.close();
    return (1);
  }
  return (0);
}



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

void UncompressZ80 (int tipo, int pc, Z80Regs * regs, FILE *fp){
  byte c, last, last2, dato;
  int cont, num, limit;
  extern tipo_mem mem;

  switch (tipo) {
      case Z80BL_V1UNCOMP:
		AS_printf (" Uncompressed V1.\n");
		fread (mem.p + pc, 0xC000, 1, fp);
	      break;
      case Z80BL_V2UNCOMP:
	      	AS_printf (" Uncompressed V2.\n");
		fread (mem.p + pc, 0x4000, 1,fp);
	      break;
       default:
	  
  limit = pc + ((tipo == Z80BL_V1COMPRE) ? 0xc000 : 0x4000); // v1=> bloque de 48K, V2 =>de 16K
/*   AS_printf (" tipo: %x PC: %x  limit:0x \n",tipo,pc, limit);
  if ((tipo == Z80BL_V1UNCOMP) || (tipo == Z80BL_V2UNCOMP)) {
      AS_printf (" Block uncompres.\n");
    for (cont = pc; cont < limit; cont++)
//      regs->RAM[cont] = fgetc (fp);
//      writemem (cont, fgetc (fp));
        *(mem.p+cont)=fgetc(fp);
	    AS_printf("pc=%x\n",cont);
  } else {
*/   
    AS_printf (" Compressed.\n");
    last = 0;
    last2 = 0;
    c = 0;
    AS_printf ("cargando desde %x hasta %x\n",pc,limit);
    do {
      last2 = last;
      last = c;
      c = fgetc (fp);
      /* detect compressed blocks */
      if ((last == 0xED) && (c == 0xED)) {
	     num = fgetc (fp);
	    /* Detect EOF */
	    if ((tipo == Z80BL_V1COMPRE) && (last2 == 0x00) && (num == 0x00))  break;
	    dato = fgetc (fp);
	    for (cont = 0; cont < num; cont++)
      *(mem.p+pc++)=dato;
	    c = 0;			/* avoid that ED ED xx yy ED zz uu shows zz uu as compressed */
	    continue;
      } else {
	     /* we found an 0xED but not the compression flag */
	  if ((last == 0xED) && (c != 0xED))
//         regs->RAM[pc++] = last;
//	       writemem (pc++, last);
           *(mem.p+pc++)=last;
	         /* If it's ED wait for the next or else to memory */
	  if (c != 0xED)
//              regs->RAM[pc++] = c;
//	            writemem (pc++, c);
              *(mem.p+pc++)=c;
      }
//    AS_printf("pc=%i\ limit=%i\n",pc,limit);
    } while (pc < limit);
  }
}

uint8_t LoadZ80 (Z80Regs * regs, FILE *fp)
{
  int f, tam, ini, sig, pag, ver = 0, hwmodel=SPECMDL_48K, np=0;
  extern tipo_hwopt hwopt;
  uint8_t buffer[87];
  fread (buffer, 87, 1, fp);
  if (buffer[12]==255) buffer[12]=1; /*as told in CSS FAQ / .z80 section */
  
  // Check file version
  if ((uint16_t) buffer[6] == 0)  {
    if ((uint16_t) buffer[30] == 23) {
      AS_printf ("Fichero Z80: Version 2.0\n");
      ver = 2;
    } else if (( (uint16_t)buffer[30] == 54 )||( (uint16_t)buffer[30] == 55 )) 
	{
      AS_printf ("Fichero Z80: Version 3.0\n");
      ver = 3;
    } else {
      AS_printf ("Fichero Z80: Version Desconocida (%04X)\n",(uint16_t) buffer[30]);
    }
    // version>=2 more hardware allowed
    switch (buffer[34]) {	/* is it 48K? */
    case 0:
	  if ((buffer[37] & 0x80) == 0x80 ){
		 AS_printf("Hardware 16K (1)\n");
		 hwmodel=SPECMDL_16K;
		 np=1;
	  } else {
        AS_printf ("Hardware 48K (2)\n");
	    hwmodel=SPECMDL_48K;
		np=3;
	  }
      break;
    case 1:
      AS_printf ("Hardware 48K + IF1 (3)\n");
	  hwmodel=SPECMDL_48KIF1;
	  break;
    case 2:
      AS_printf ("Hardware 48K + SAMRAM (4)\n");
      return (-1);
	  break;
    case 3:
      if (ver == 2) {
	     if ((buffer[37] & 0x80) == 0x80 ){
		    AS_printf("Hardware +2 128K (5)\n");
		    hwmodel=SPECMDL_PLUS2;
			np=8;
	     } else {
            AS_printf ("Hardware 128K (6)\n");
	        hwmodel=SPECMDL_128K;
			np=8;
	     }
      } else {
	     AS_printf ("48K + M.G.T. (7)\n");
	     return (-1);
      }
	  break;
    case 4:
      if (ver == 2) {
	    AS_printf ("Hardware 128K + IF1 (8)\n");
	    return (-1);
      } else {
	     if ((buffer[37] & 0x80) == 0x80 ){
		    AS_printf("Hardware +2 128K (9)\n");
		    hwmodel=SPECMDL_PLUS2;
			np=8;
	     } else {
            AS_printf ("Hardware 128K (A)\n");
	        hwmodel=SPECMDL_128K;
			np=8;
	     }
      }
	  break;
    case 5:
      AS_printf ("Hardware 128K + IF1 (B)\n");
      return (-1);
	  break;
    case 6:
      AS_printf ("Hardware 128K + M.G.T (C)\n");
      return (-1);
	  break;
    case 7:
	    if ((buffer[37] & 0x80) == 0x80 ){
		   AS_printf("Hardware +2A 128K (D)\n");
        	   hwmodel=SPECMDL_PLUS3;
		   np=8;
	    } else {
           AS_printf ("Hardware Spectrum +3 (E)\n");
                   hwmodel=SPECMDL_PLUS3;
		   np=8;
		}
	    break;
    case 8:
	     if ((buffer[37] & 0x80) == 0x80 ){
		    AS_printf("Hardware +2A 128K (F)\n");
                    hwmodel=SPECMDL_PLUS3;
		    np=8;
	     } else {
                   AS_printf ("Hardware Spectrum +3 (xzx deprecated)(10)\n");
                   hwmodel=SPECMDL_PLUS3;
		   np=8;
	     }
		 break;
    case 9:
      AS_printf ("Pentagon 128K (11)\n");
	  np=8;
      return (-1);
	  break;
    case 10:
      AS_printf ("Hardware Scorpion (12)\n");
      np=16;
      return (-1);
	  break;
    case 11:
      AS_printf ("Hardware Didaktik-Kompakt (13)\n");
      return (-1);
	  break;
    case 12:
      AS_printf ("Hardware Spectrum +2 128K (14)\n");
     hwmodel=SPECMDL_PLUS2;
     np=8;
	 break;
    case 13:
          AS_printf ("Hardware Spectrum +2A 128K (15)\n");
	  hwmodel=SPECMDL_PLUS3;
  	  np=8;
	  break;
    case 14:
      AS_printf ("Hardware Timex TC2048 (16)\n");
      return (-1);
	  break;
    case 64:
      AS_printf ("Hardware Inves Spectrum + 48K (Aspectrum Specific)(17)\n");
      hwmodel=SPECMDL_INVES;     
      np=4;
	  break;
    case 65:
      AS_printf ("Hardware Spectrum 128K Spanish (Aspectrum Specific)(18)\n");
     hwmodel=SPECMDL_128K;
     np=8;
	 break;
    case 128:
      AS_printf ("Hardware Timex TC2068 (19)\n");
      return (-1);
	  break;
    default:
      AS_printf ("Unknown hardware, %x\n", buffer[34]);
      return (-1);
	  break;
    }
/*
	if (hwmodel!=SPECMDL_48K) {
		AS_printf("Hardware no soporta carga de .Z80\n");
		return (-1);
	}
*/	
    if (hwopt.hw_model != hwmodel ) { // cambiamos de arquitectura si no es correcta.
      end_spectrum ();
      if (init_spectrum (hwmodel, "")==1) return (1); // si falla la arquitectura no sigo
    }
	
	if (np==0) AS_printf("YEPA te has colado np=0\n");
		
    sig = 30 + 2 + buffer[30];
    fseek (fp, sig, SEEK_SET);
    for (f = 0; f < np; f++) {
      tam = fgetc (fp);
      tam = tam + (fgetc (fp) << 8);
      pag = fgetc (fp);
      sig = sig + tam + 3;
      AS_printf (" pagina a cargar es: %i, tama�o:%x\n", pag, tam);
	switch (hwmodel) {
		 case SPECMDL_16K:
		 case SPECMDL_48K:
			  switch (pag) {
				  case 4:
					ini = 0x8000;
					break;
				  case 5:
					ini = 0xc000;
					break;
				  case 8:
					ini = 0x4000;
					break;
				  default:
					ini = 0x4000;
					AS_printf
					  ("Algo raro pasa con ese Snapshot, remitalo al autor para debug\n");
					break;
				  }
				  break;
		 case SPECMDL_128K:
		 case SPECMDL_PLUS2:
		 case SPECMDL_PLUS3:
			 switch (pag) {
				  case 3:
					ini = 0x0000;
					break;
				  case 4:
					ini = 0x4000;
					break;
				  case 5:
					ini = 0x8000;
					break;
				  case 6:
					ini = 0xc000;
					break;
				  case 7:
					ini = 0x10000;
					break;
				  case 8:
					ini = 0x14000;
					break;
				  case 9:
					ini = 0x18000;
					break;
				  case 10:
					ini = 0x1c000;
					break;
				  default:
          ini = 0x04000;
          AS_printf("Algo raro pasa con ese Snapshot, remitalo al autor para debug\n");
			    break;																				
			  }			      
			 break;
		 case SPECMDL_INVES:
			  switch (pag) {
				  case 4:
					ini = 0x8000;
					break;
				  case 5:
					ini = 0xc000;
					break;
				  case 8:
					ini = 0x4000;
					break;
				  case 0:
					ini = 0x10000;
					break;
				  default:
          ini = 0x4000;
          AS_printf ("Algo raro pasa con ese Snapshot, remitalo al autor para debug\n");
					break;
			 }
			 break;
	 }
    UncompressZ80 ((tam == 0xffff ? Z80BL_V2UNCOMP : Z80BL_V2COMPRE),ini, regs, fp);
    }
    regs->PC.B.l = buffer[32];
    regs->PC.B.h = buffer[33];
    if ((hwmodel==SPECMDL_128K) || (hwmodel==SPECMDL_PLUS2)) {
	      hwopt.BANKM=0x00; /* need to clear lock latch or next dont work. */
        outbankm_128k(buffer[35]);
    }
    if (hwmodel==SPECMDL_PLUS3) {
	      hwopt.BANKM=0x00; /* need to clear lock latch or next dont work. */
        outbankm_p37(buffer[35]);
        outbankm_p31(buffer[86]);
    }
    
    // aki abria que a�adir lo del sonido claro.
    
  } else {
    AS_printf ("Fichero Z80: Version 1.0\n");

    if ( hwopt.hw_model != SPECMDL_48K ) { // V1 es siempre 48K camb. arquit. si no es correcta.
        end_spectrum ();
        init_spectrum (SPECMDL_48K, "");
    }
    
    if (buffer[12] & 0x20) {
      AS_printf ("Fichero Z80: Comprimido\n");
      fseek (fp, 30, SEEK_SET);
      UncompressZ80 (Z80BL_V1COMPRE, 0x4000, regs, fp);
    } else {
      AS_printf ("Fichero Z80: Sin comprimir\n");
      fseek (fp, 30, SEEK_SET);
      UncompressZ80 (Z80BL_V1UNCOMP, 0x4000, regs, fp);
    }
    regs->PC.B.l = buffer[6];
    regs->PC.B.h = buffer[7];
  }

  regs->AF.B.h = buffer[0];
  regs->AF.B.l = buffer[1];
  regs->BC.B.l = buffer[2];
  regs->BC.B.h = buffer[3];
  regs->HL.B.l = buffer[4];
  regs->HL.B.h = buffer[5];
  regs->SP.B.l = buffer[8];
  regs->SP.B.h = buffer[9];
  regs->I = buffer[10];
  regs->R.B.l = buffer[11];
  regs->R.B.h = 0;
  hwopt.BorderColor = ((buffer[12] & 0x0E) >> 1);
  regs->DE.B.l = buffer[13];
  regs->DE.B.h = buffer[14];
  regs->BCs.B.l = buffer[15];
  regs->BCs.B.h = buffer[16];
  regs->DEs.B.l = buffer[17];
  regs->DEs.B.h = buffer[18];
  regs->HLs.B.l = buffer[19];
  regs->HLs.B.h = buffer[20];
  regs->AFs.B.h = buffer[21];
  regs->AFs.B.l = buffer[22];
  regs->IY.B.l = buffer[23];
  regs->IY.B.h = buffer[24];
  regs->IX.B.l = buffer[25];
  regs->IX.B.h = buffer[26];
  regs->IFF1 = buffer[27] & 0x01;
  regs->IFF2 = buffer[28] & 0x01;
  regs->IM = buffer[29] & 0x03;

  return (0);
}

/*-----------------------------------------------------------------
 char LoadSP( Z80Regs *regs, FILE *fp );
 This loads a .SP file from disk to the Z80 registers/memory.
------------------------------------------------------------------*/
uint8_t LoadSP (Z80Regs * regs, FILE *fp, tipo_mem &mem){
  extern tipo_hwopt hwopt;
  unsigned short length, start, sword;
  int f;
  uint8_t buffer[80];		// �por que 80 si leemos 38?
  fread (buffer, 38, 1, fp);

  /* load the .SP header: */
  length = (buffer[3] << 8) + buffer[2];
  start = (buffer[5] << 8) + buffer[4];
  regs->BC.B.l = buffer[6];
  regs->BC.B.h = buffer[7];
  regs->DE.B.l = buffer[8];
  regs->DE.B.h = buffer[9];
  regs->HL.B.l = buffer[10];
  regs->HL.B.h = buffer[11];
  regs->AF.B.l = buffer[12];
  regs->AF.B.h = buffer[13];
  regs->IX.B.l = buffer[14];
  regs->IX.B.h = buffer[15];
  regs->IY.B.l = buffer[16];
  regs->IY.B.h = buffer[17];
  regs->BCs.B.l = buffer[18];
  regs->BCs.B.h = buffer[19];
  regs->DEs.B.l = buffer[20];
  regs->DEs.B.h = buffer[21];
  regs->HLs.B.l = buffer[22];
  regs->HLs.B.h = buffer[23];
  regs->AFs.B.l = buffer[24];
  regs->AFs.B.h = buffer[25];
  regs->R.W = 0;
  regs->R.B.l = buffer[26];
  regs->I = buffer[27];
  regs->SP.B.l = buffer[28];
  regs->SP.B.h = buffer[29];
  regs->PC.B.l = buffer[30];
  regs->PC.B.h = buffer[31];
  hwopt.BorderColor = buffer[34];
  sword = (buffer[37] << 8) | buffer[36];
  AS_printf ("\nSP_PC = %04X, SP_START =  %d,  SP_LENGTH = %d\n", regs->PC,
	    start, length);

  /* interrupt mode */
  regs->IFF1 = regs->IFF2 = 0;
  if (sword & 0x4)
    regs->IFF2 = 1;
  if (sword & 0x8)
    regs->IM = 0;
  else {
    if (sword & 0x2)
      regs->IM = 2;
    else
      regs->IM = 1;
  }
  if (sword & 0x1)
    regs->IFF1 = 1;


  if (sword & 0x16) {
    AS_printf ("\n\nPENDING INTERRUPT!!\n\n");
  } else {
    AS_printf ("\n\nno pending interrupt.\n\n");
  }

//FIXME leer todo a la vez.
  for (f = 0; f <= length; f++)
    if (start + f < 65536)
//      regs->RAM[start + f] = fgetc (fp);
//      writemem (start + f, fgetc (fp));
      *(mem.p+start + f)=fgetc (fp);

  return (0);
}


/*-----------------------------------------------------------------
 char LoadSNA( Z80Regs *regs, char *filename );
 This loads a .SNA file from disk to the Z80 registers/memory.
------------------------------------------------------------------*/
int Load_SNA (Z80Regs * regs, const char *filename){
  File f;
  uint8_t page;
  uint8_t buffer[27];
  uint8_t unbyte;
  int model;
  //FIXIT no extern
  extern tipo_hwopt hwopt;
  extern tipo_mem mem;
  AS_print("Cargamos un SNA (by filename)\n");
  f=SPIFFS.open(filename,FILE_READ);
  if (!f) {
    AS_print("Algo fallo cargando el SNA\n");
    return 1;
  }
  if (f.size() == 49179) model = SPECMDL_48K ;  // es un 48Kb
  else if (f.size() == 16411) model = SPECMDL_16K ; //16Kb UNSUPPORTED!!
  else model = SPECMDL_128K ;

  if (hwopt.hw_model != model) {  // comprobamos si es el modelo correcto.
      end_spectrum ();
      init_spectrum (model, "");
  }
  
  f.seek (0, SeekSet);
  f.read (buffer, 27);
  regs->I = buffer[0];
  regs->HLs.B.l = buffer[1];
  regs->HLs.B.h = buffer[2];
  regs->DEs.B.l = buffer[3];
  regs->DEs.B.h = buffer[4];
  regs->BCs.B.l = buffer[5];
  regs->BCs.B.h = buffer[6];
  regs->AFs.B.l = buffer[7];
  regs->AFs.B.h = buffer[8];
  regs->HL.B.l = buffer[9];
  regs->HL.B.h = buffer[10];
  regs->DE.B.l = buffer[11];
  regs->DE.B.h = buffer[12];
  regs->BC.B.l = buffer[13];
  regs->BC.B.h = buffer[14];
  regs->IY.B.l = buffer[15];
  regs->IY.B.h = buffer[16];
  regs->IX.B.l = buffer[17];
  regs->IX.B.h = buffer[18];
  regs->IFF1 = regs->IFF2 = (buffer[19] & 0x04) >> 2;
  regs->R.W = buffer[20];
  regs->AF.B.l = buffer[21];
  regs->AF.B.h = buffer[22];
  regs->SP.B.l = buffer[23];
  regs->SP.B.h = buffer[24];
  regs->IM = buffer[25];
  hwopt.BorderColor = buffer[26];

  if (model=SPECMDL_48K) {  // es un 48Kb
    f.read (mem.p + 16384, 0x4000 * 3);
    regs->PC.B.l = z80_peek(regs->SP.W);
    regs->SP.W++;
    regs->PC.B.h = z80_peek(regs->SP.W);
    regs->SP.W++;

  } else if (model=SPECMDL_16K) {  // es un 16Kb UNSUPPORTED
    f.read (mem.p + 16384, 0x4000 * 1);
    regs->PC.B.l = z80_peek(regs->SP.W);
    regs->SP.W++;
    regs->PC.B.h = z80_peek(regs->SP.W);
    regs->SP.W++;

  } else {      // es un 128Kb FIXME hay SNA de 16Kb
    f.seek (49179, SeekSet);
    f.read(&unbyte,1); //FIXME seguro que hay formas mejores
    regs->PC.B.l = unbyte;
    f.read(&unbyte,1);
    regs->PC.B.h = unbyte;
    f.read(&page,1);
    hwopt.BANKM = 0x00;   /* unlock 128K pagging */
    outbankm_128k (page);
    page &= 0x07;
    // ahora empezamos a rellenar ram. 
    f.seek (27, SeekSet); 
    f.read (mem.p + (5 * 0x4000)   , 0x4000);
    f.read (mem.p + (2 * 0x4000)   , 0x4000);
    f.read (mem.p + (page * 0x4000), 0x4000);
    //resto de paginas.
    f.seek (49183, SeekSet);
    if (page != 0) f.read (mem.p + (0 * 0x4000), 0x4000);
    if (page != 1) f.read (mem.p + (1 * 0x4000), 0x4000);
    if (page != 3) f.read (mem.p + (3 * 0x4000), 0x4000);
    if (page != 4) f.read (mem.p + (4 * 0x4000), 0x4000);
    if (page != 6) f.read (mem.p + (6 * 0x4000), 0x4000);
    if (page != 7) f.read (mem.p + (7 * 0x4000), 0x4000);
  }
  f.close();
  return (0);
}


/*-----------------------------------------------------------------
 char SaveSNA( Z80Regs *regs, FILE *fp );
 This saves a .SNA file from the Z80 registers/memory to disk.
------------------------------------------------------------------*/
uint8_t SaveSNA (Z80Regs * regs, FILE * fp){
  extern tipo_hwopt hwopt;
  extern tipo_mem mem;
  unsigned char sptmpl, sptmph;
//  int c;

// SNA solo esta soportado en 48K, 128K y +2

  if ((hwopt.hw_model != SPECMDL_48K) && (hwopt.hw_model != SPECMDL_128K)) {
    AS_print ("El modelo de Spectrum utilizado");
    AS_print ("No permite grabar el snapshot en formato SNA");
	  AS_print ("Utilize otro tipo de archivo (extension)");
    return 1;
  }

  /* save the .SNA header */
  fputc (regs->I, fp);
  fputc (regs->HLs.B.l, fp);
  fputc (regs->HLs.B.h, fp);
  fputc (regs->DEs.B.l, fp);
  fputc (regs->DEs.B.h, fp);
  fputc (regs->BCs.B.l, fp);
  fputc (regs->BCs.B.h, fp);
  fputc (regs->AFs.B.l, fp);
  fputc (regs->AFs.B.h, fp);
  fputc (regs->HL.B.l, fp);
  fputc (regs->HL.B.h, fp);
  fputc (regs->DE.B.l, fp);
  fputc (regs->DE.B.h, fp);
  fputc (regs->BC.B.l, fp);
  fputc (regs->BC.B.h, fp);
  fputc (regs->IY.B.l, fp);
  fputc (regs->IY.B.h, fp);
  fputc (regs->IX.B.l, fp);
  fputc (regs->IX.B.h, fp);
  fputc (regs->IFF1 << 2, fp);
  fputc (regs->R.W & 0xFF, fp);
  fputc (regs->AF.B.l, fp);
  fputc (regs->AF.B.h, fp);

//  sptmpl = Z80MemRead (regs->SP.W - 1, regs);
//  sptmph = Z80MemRead (regs->SP.W - 2, regs);
  sptmpl = z80_peek (regs->SP.W - 1);
  sptmph = z80_peek (regs->SP.W - 2);

  if (hwopt.hw_model == SPECMDL_48K) {	// code for the 48K version.
    AS_printf("Guardando SNA 48K\n");
	 /* save PC on the stack */
//    Z80MemWrite (--(regs->SP.W), regs->PC.B.h, regs);
//    Z80MemWrite (--(regs->SP.W), regs->PC.B.l, regs);
    z80_poke (--(regs->SP.W), regs->PC.B.h);
    z80_poke (--(regs->SP.W), regs->PC.B.l);

    fputc (regs->SP.B.l, fp);
    fputc (regs->SP.B.h, fp);
    fputc (regs->IM, fp);
    fputc (hwopt.BorderColor, fp);

    /* save the RAM contents */
//  fwrite (regs->RAM + 16384, 48 * 1024, 1, fp);
    fwrite (mem.p + 16384, 0x4000 * 3, 1, fp);

    /* restore the stack and the SP value */
    regs->SP.W += 2;
//    Z80MemWrite (regs->SP.W - 1, sptmpl, regs);
//    Z80MemWrite (regs->SP.W - 2, sptmph, regs);
    z80_poke (regs->SP.W - 1, sptmpl);
    z80_poke (regs->SP.W - 2, sptmph);

  } else {			// code for the 128K version
    AS_printf("Guardando SNA 128K\n");
    fputc (regs->SP.B.l, fp);
    fputc (regs->SP.B.h, fp);
    fputc (regs->IM, fp);
    fputc (hwopt.BorderColor, fp);

    // volcar la ram  
//  Volcar 5
//  for (c=mem.ro[1];(c<mem.ro[1]+0x4000);c++) fputc (mem.p[c],fp);
    fwrite (mem.p + mem.ro[1], 0x4000, 1, fp);

// Volcar 2
//  for (c=mem.ro[2];(c<mem.ro[2]+0x4000);c++) fputc (mem.p[c],fp);
    fwrite (mem.p + mem.ro[2], 0x4000, 1, fp);

// volcar N
//  for (c=mem.ro[3];(c<mem.ro[3]+0x4000);c++) fputc (mem.p[c],fp);
    fwrite (mem.p + mem.ro[3], 0x4000, 1, fp);


// resto de cosas
    fputc (regs->PC.B.l, fp);
    fputc (regs->PC.B.h, fp);
    fputc (hwopt.BANKM, fp);
    fputc (0, fp);		// TR-DOS rom paged (1) or not (0)

    // volcar el resto de ram.

//  Volcar 0 si no en (4)  
    if (mem.ro[3] != (0 * mem.sp))
//    for (c=0*mem.sp;c<((0*mem.sp)+0x4000);c++) fputc (mem.p[c],fp);
      fwrite (mem.p + (0 * mem.sp), 0x4000, 1, fp);

//  Volcar 1 si no en (4)  
    if (mem.ro[3] != (1 * mem.sp))
//    for (c=1*mem.sp;c<((1*mem.sp)+0x4000);c++) fputc (mem.p[c],fp);
      fwrite (mem.p + (1 * mem.sp), 0x4000, 1, fp);

//  Volcar 3 si no en (4)  
    if (mem.ro[3] != (3 * mem.sp))
//    for (c=3*mem.sp;c<((3*mem.sp)+0x4000);c++) fputc (mem.p[c],fp);
      fwrite (mem.p + (3 * mem.sp), 0x4000, 1, fp);

//  Volcar 4 si no en (4)  
    if (mem.ro[3] != (4 * mem.sp))
//    for (c=4*mem.sp;c<((4*mem.sp)+0x4000);c++) fputc (mem.p[c],fp);
      fwrite (mem.p + (4 * mem.sp), 0x4000, 1, fp);

//  Volcar 6 si no en (4)  
    if (mem.ro[3] != (6 * mem.sp))
//    for (c=6*mem.sp;c<((6*mem.sp)+0x4000);c++) fputc (mem.p[c],fp);
      fwrite (mem.p + (6 * mem.sp), 0x4000, 1, fp);

//  Volcar 7 si no en (4)  
    if (mem.ro[3] != (7 * mem.sp))
//    for (c=7*mem.sp;c<((7*mem.sp)+0x4000);c++) fputc (mem.p[c],fp);
      fwrite (mem.p + (7 * mem.sp), 0x4000, 1, fp);

  }
  return (0);
}

uint8_t SaveSP (Z80Regs * regs, FILE * fp){
  extern tipo_mem mem;
  extern tipo_hwopt hwopt;
  // save the .SP header 
  fputc ('S', fp);
  fputc ('P', fp);
  fputc (00, fp);
  fputc (0xC0, fp);		// 49152
  fputc (00, fp);
  fputc (0x40, fp);		//16384
  // save the state
  fputc (regs->BC.B.l, fp);
  fputc (regs->BC.B.h, fp);
  fputc (regs->DE.B.l, fp);
  fputc (regs->DE.B.h, fp);
  fputc (regs->HL.B.l, fp);
  fputc (regs->HL.B.h, fp);
  fputc (regs->AF.B.l, fp);
  fputc (regs->AF.B.h, fp);
  fputc (regs->IX.B.l, fp);
  fputc (regs->IX.B.h, fp);
  fputc (regs->IY.B.l, fp);
  fputc (regs->IY.B.h, fp);
  fputc (regs->BCs.B.l, fp);
  fputc (regs->BCs.B.h, fp);
  fputc (regs->DEs.B.l, fp);
  fputc (regs->DEs.B.h, fp);
  fputc (regs->HLs.B.l, fp);
  fputc (regs->HLs.B.h, fp);
  fputc (regs->AFs.B.l, fp);
  fputc (regs->AFs.B.h, fp);
  fputc (regs->R.B.l, fp);
  fputc (regs->I, fp);
  fputc (regs->SP.B.l, fp);
  fputc (regs->SP.B.h, fp);
  fputc (regs->PC.B.l, fp);
  fputc (regs->PC.B.h, fp);
  fputc (0, fp);
  fputc (0, fp);
  fputc (hwopt.BorderColor, fp);
  fputc (0, fp);
  // Estado, pendiente por poner:  Si hay una int pendiente (bit 4), valor de flash (bit 5).
  fputc (0, fp);
  fputc ((((regs->IFF2) << 2) + ((regs->IM) == 2 ? 0x02 : 0x00) + regs->IFF1),
	 fp);
  // Save the ram
//  fwrite (regs->RAM + 16384, 48 * 1024, 1, fp);
  fwrite (mem.p + 16384, 0x4000 * 3, 1, fp);

  return (0);
}

void writeZ80block(int block, int offset, FILE *fp)
{
	extern tipo_mem mem;
	fputc(0xff,fp); fputc(0xff,fp);
	fputc((byte)block,fp);
  fwrite (mem.p + offset, 0x4000 , 1, fp);
}

uint8_t SaveZ80 (Z80Regs * regs, FILE * fp){
  extern tipo_mem mem;
  extern tipo_hwopt hwopt;
  int c;

	// 48K  .z80 are saved as ver 1.45 
  AS_printf ("Guardando Z80...\n");
  fputc (regs->AF.B.h, fp);
  fputc (regs->AF.B.l, fp);
  fputc (regs->BC.B.l, fp);
  fputc (regs->BC.B.h, fp);
  fputc (regs->HL.B.l, fp);
  fputc (regs->HL.B.h, fp);

if (hwopt.hw_model==SPECMDL_48K) {  // si no es 48K entonces V 3.0
  AS_printf ("...de 48K\n");
  	fputc (regs->PC.B.l, fp);
  	fputc (regs->PC.B.h, fp);
} else {
   AS_printf ("...de NO 48K\n");
	fputc(0,fp);
	fputc(0,fp);
}

  fputc (regs->SP.B.l, fp);
  fputc (regs->SP.B.h, fp);
  fputc (regs->I, fp);
  fputc ((regs->R.B.l & 0x7F), fp);
  fputc ((((regs->R.B.l & 0x80) >> 7) | ((hwopt.BorderColor & 0x07) << 1)),
	 fp); //Datos sin comprimir por defecto.
  fputc (regs->DE.B.l, fp);
  fputc (regs->DE.B.h, fp);
  fputc (regs->BCs.B.l, fp);
  fputc (regs->BCs.B.h, fp);
  fputc (regs->DEs.B.l, fp);
  fputc (regs->DEs.B.h, fp);
  fputc (regs->HLs.B.l, fp);
  fputc (regs->HLs.B.h, fp);
  fputc (regs->AFs.B.h, fp);
  fputc (regs->AFs.B.l, fp);
  fputc (regs->IY.B.l, fp);
  fputc (regs->IY.B.h, fp);
  fputc (regs->IX.B.l, fp);
  fputc (regs->IX.B.h, fp);
  fputc (regs->IFF1, fp);
  fputc (regs->IFF2, fp);
  fputc ((regs->IM & 0x7), fp); // issue 2 and joystick cfg not saved.

if (hwopt.hw_model==SPECMDL_48K) {
	//  fwrite (regs->RAM + 16384, 48 * 1024, 1, fp);
   fwrite (mem.p + 16384, 0x4000 * 3, 1, fp);
} else { // aki viene la parte de los 128K y demas
	fputc( (hwopt.hw_model==SPECMDL_PLUS3 ? 55:54 ),fp);
	fputc(0,fp);
  	fputc (regs->PC.B.l, fp);
  	fputc (regs->PC.B.h, fp);
	switch (hwopt.hw_model) {
		case SPECMDL_16K:
            		fputc(0,fp);
			fputc(0,fp);
    			break;
		case SPECMDL_INVES:
			fputc(64,fp);
            		fputc(0,fp);
			break;
		case SPECMDL_128K:
			fputc(4,fp);
                  	fputc(hwopt.BANKM,fp);
    			break;
		case SPECMDL_PLUS2:
			fputc(12,fp);
            		fputc(hwopt.BANKM,fp);
     			break;
		case SPECMDL_PLUS3: // as there is no disk emuation, alwais save as +2A
			fputc(13,fp);
			fputc(hwopt.BANKM,fp);
		default:
			AS_printf("ERROR: HW Type Desconocido, nunca deberias ver esto.\n");
			fputc(0,fp);
			fputc(0,fp);
		 	break;
								 
	}
    fputc(0,fp); // if1 not paged.
    if (hwopt.hw_model==SPECMDL_16K) fputc(0x81,fp); 
    else fputc(1,fp); //siempre emulamos el registro R (al menos que yo sepa)
		    
    for (c=0;c<(1+16+3+1+4+20+3);c++) fputc(0,fp);
   
    if (hwopt.hw_model==SPECMDL_PLUS3) fputc(hwopt.BANK678,fp);
    
    switch (hwopt.hw_model) {
		case SPECMDL_16K:
            writeZ80block(8,0x4000,fp);
    		break;
		case SPECMDL_INVES:
            writeZ80block(8,0x4000,fp);
            writeZ80block(4,0x8000,fp);
            writeZ80block(5,0xC000,fp);
            writeZ80block(0,0x10000,fp);
			break;
		case SPECMDL_128K:
		case SPECMDL_PLUS2:
            writeZ80block(3,  0x0000,fp);
            writeZ80block(4,  0x4000,fp);
            writeZ80block(5,  0x8000,fp);
            writeZ80block(6,  0xC000,fp);
            writeZ80block(7,  0x10000,fp);
            writeZ80block(8,  0x14000,fp);
            writeZ80block(9,  0x18000,fp);
            writeZ80block(10,0x1C000,fp);
     		break;
	}
}
return (0);
}

uint8_t LoadSCR (Z80Regs * regs, FILE * fp){
  int i;
  // FIX use fwrite
  // FIX2 must be the actual video page
  for (i = 0; i < 6912; i++)
    z80_poke (16384 + i, fgetc (fp));
  return 0;
}



/*-----------------------------------------------------------------
 char SaveSCR( Z80Regs *regs, FILE *fp );
 This saves a .SCR file from the Spectrum videomemory.
------------------------------------------------------------------*/
uint8_t SaveSCR (Z80Regs * regs, FILE * fp){
  int i;
  extern tipo_hwopt hwopt;
  /* Save the contents of VideoRAM to file: write the 6192 bytes
   * starting on the memory address 16384 */
  //FIXME: user  fwrite in the actual videopage
  for (i = 0; i < 6912; i++)
    fputc (readvmem (i, hwopt.videopage), fp);
//    fputc (Z80MemRead (16384 + i, regs), fp);

  return (0);
}


/*-----------------------------------------------------------------
------------------------------------------------------------------*/
/*-----------------------------------------------------------------
	Rutinas Genericas para el manejo de Cintas
------------------------------------------------------------------*/


int TZX_type;

FILE *InitTape (FILE *fp){
/* FIXME: tape support
  // extern tipo_emuopt emuopt;
  //  AS_printf("Z80 InitTape llamado\n");

  if (fp = SPIFFS.open(emuopt.tapefile, "rb"))
    return fp;
  //AS_printf("InitTape - abierto:%x\n",fp);

  switch (typeoffile (emuopt.tapefile)) {
  case TYPE_TZX:
    TZX_init (fp);
    break;
  case TYPE_TAP:
    TAP_init (fp);
    break;
  }

  return fp;
*/
  return NULL;
}

int typeoffile (const char *name){
  int top;
  //AS_printf("typeoffile llamado con %s\n",name);
  for (top = 0; name[top] != 0; top++);
  //      AS_printf("letra %c ascii: %i, top=%i\n",name[top],name[top],top);
  //AS_printf("encontrado en %i, y extension es %s\n",top, &(name[top-4]) );

  if (strcmp (&(name[top - 4]), ".TAP") == 0)
    return TYPE_TAP;
  if (strcmp (&(name[top - 4]), ".tap") == 0)
    return TYPE_TAP;
  if (strcmp (&(name[top - 4]), ".TZX") == 0)
    return TYPE_TZX;
  if (strcmp (&(name[top - 4]), ".tzx") == 0)
    return TYPE_TZX;
  if (strcmp (&(name[top - 4]), ".Z80") == 0)
    return TYPE_Z80;
  if (strcmp (&(name[top - 4]), ".z80") == 0)
    return TYPE_Z80;
  if (strcmp (&(name[top - 4]), ".SNA") == 0)
    return TYPE_SNA;
  if (strcmp (&(name[top - 4]), ".sna") == 0)
    return TYPE_SNA;
  if (strcmp (&(name[top - 3]), ".SP") == 0)
    return TYPE_SP;
  if (strcmp (&(name[top - 3]), ".sp") == 0)
    return TYPE_SP;
  if (strcmp (&(name[top - 4]), ".SCR") == 0)
    return TYPE_SCR;
  if (strcmp (&(name[top - 4]), ".scr") == 0)
    return TYPE_SCR;
  return TYPE_NULL;
}

/*-----------------------------------------------------------------
 char LoadTAP( Z80Regs *regs, FILE *fp );
 This loads a .TAP file from disk to the Z80 registers/memory,
 almost at zero speed :-)

    WARNING: EXPERIMENTAL X'DDDDDD
------------------------------------------------------------------*/
uint8_t LoadTAP (Z80Regs * regs, FILE * fp){
  // AS_printf("Llamada LoadTAP generica\n");
  // AS_printf("LoadTap:%x\n",fp);
  switch (TZX_type) {
  case TYPE_TAP:
    return TAP_loadblock (regs, fp);
    break;
  case TYPE_TZX:
    return TZX_loadblock (regs, fp);
    break;
  default:
    AS_printf ("Cargando algo que ni es TAP ni TZX, ���QUE CHUNGO!!!\n");
    break;
  }
  return 0;
}

uint8_t RewindTAP (Z80Regs * regs, FILE * fp){
  AS_printf ("Llamada RewindTAP generica\n");
  //              AS_printf("LoadTap:%x\n",fp);
  switch (TZX_type) {
  case TYPE_TAP:
    TAP_rewind (fp);
    break;
  case TYPE_TZX:
    TZX_rewind ();
    break;
  default:
    AS_printf ("Rebobinando algo que ni es TAP ni TZX, ���QUE CHUNGO!!!\n");
    break;
  }
  return 0;
}

uint8_t TAP_loadblock (Z80Regs * regs, FILE * fp){
  int blow, bhi, bytes, f, howmany,load;
  unsigned int where;
  AS_printf ("Llamada TAP_loadblock\n");
  AS_printf ("\n--- Trying to load from tape: reached %04Xh ---\n", regs->PC.W);
  AS_printf ("On enter:  A=%d, IX=%d, DE=%d\n", regs->AF.B.h, regs->IX.W, regs->DE.W);
/*
  // auto tape-rewind function on end-of-file
  if( feof(fp) )
  {
      AS_printf("END OF FILE: Tape rewind to 0...\n");
      fseek( fp, 0, SEEK_SET );
  }
*/

  blow = fgetc (fp);
  bhi = fgetc (fp);
  bytes = (bhi << 8) | blow;
  AS_printf ("%d bytes to read on file, DE=%d requested.\n", bytes - 2, regs->DE.W);
  where = regs->IX.W;
  load=   (regs->AF.B.l) & C_FLAG ;
  fgetc (fp);			/* read flag type and ignore it */
	/* FIXME No deberiamos ignorarlo �saltar al siguiente? */
  /* determine how many bytes to read. If there are less bytes in
     the tap block than the requested DE bytes, generate the read
     error by setting the C_FLAG on F... */
  howmany = regs->DE.W;
  if (bytes - 2 < howmany) {
    howmany = bytes - 2;
    (regs->AF.B.l &= ~(C_FLAG));
    AS_printf ("Generating a tape load error (tapbytes < DE)...\n");
    regs->IX.W += bytes - 2;
  } else  /* FIXME Deberia cambiarse la memoria hasta el momento en que se genera el error */
    regs->IX.W += regs->DE.W;

  for (f = 0; f < howmany; f++) {
    switch (load)
    {
	    case 0x00: // verificando, si en algun momento no coincide, flag arriba.
		    if (fgetc(fp)!= z80_peek(where+f))
			    (regs->AF.B.l &= ~(C_FLAG));
		    break;
	    case C_FLAG : // cargando
		    z80_poke (where + f, fgetc (fp));
		    break;
    }
    if (howmany == 17 && f <= 10)
      putchar (z80_peek (where + f));
  }

  if (howmany == 17) {
    AS_printf ("\n");
  }
  fgetc (fp);			/* read checksum (and ignore it :-) */

  /* if load was successful, reset C_FLAG */
  if (howmany == regs->DE.W)
    (regs->AF.B.l |= (C_FLAG));

  regs->DE.W = 0;
  AS_printf ("On exit:  A=%d, IX=%d, DE=%d, F=%02Xh, PC=%04Xh\n",
	    regs->AF.B.h, regs->IX.W, regs->DE.W, regs->AF.B.l,
	    (z80_peek (regs->SP.W + 1) << 8) | z80_peek (regs->SP.W));
  return (0);
}


/* Rebobina una Cinta .TAP */
uint8_t
TAP_rewind (FILE * fp) {
  // falta mirar si hay cabezera
  fseek (fp, 0, SEEK_SET);
  return 0;
}

/* las cintas .tap no necesitan inicializacion */
uint8_t TAP_init (FILE * fp){
  TZX_type = TYPE_TAP;
  return 0;
}

/* Definiciones de variables que afectan a la carga de TZX */

int TZX_numofblocks;
int TZX_actualblock;
int TZX_index_nblock;
struct TZX_block_struct
{
  int id;
  int sup;
  long offset;
};
struct TZX_block_struct *TZX_index;

/* Inicializa una cinta .TZX */
uint8_t TZX_init (FILE * fp){
  //comprueba si tiene la firma de un tzx
  char cadena[9];
  int vma, vme;
  AS_printf ("Iniciando TZX\n");
  if (fgets (cadena, 9, fp) != NULL)
    if (memcmp (cadena, "ZXTape!\0x1A", 7) != 0)
      return -1;		// no es un archivo tzx.

  //comprueba la version
  vma = fgetc (fp);
  vme = fgetc (fp);
  AS_printf ("Encontrada signature TZX, Version: %i.%i\n", vma, vme);

  if (vma > 1) {
    AS_printf
      ("Version no soportada, a dia 10/3/2003 significa TZX erroneo\n");
    return -1;
  }
  // Genera indice. 
  if (TZX_genindex (fp) == 0)
    return -1;			// el tzx era valido pero no tenia bloques validos.
  // encontro un bloque valido.
  TZX_actualblock = 0;
  TZX_type = TYPE_TZX;
  return 0;
}

/* carga un bloque desde un .TZX */
uint8_t TZX_loadblock (Z80Regs * regs, FILE * fp){
  int where, flags, howmany, bytes, f;
  int enviado = FALSE;

  AS_printf ("Llamada TZX_loadblock, bloque actual: %i de %i\n",
	    TZX_actualblock, TZX_numofblocks);
  // AS_printf("TZX_loadblock:%x\n",fp);
  for (; TZX_actualblock < TZX_numofblocks; TZX_actualblock++) {
    if (TZX_actualblock >= TZX_numofblocks)
      break;			// el bucle for se comprueba siempre una vez.
    AS_printf ("Evaluando bloque: %i - ", TZX_actualblock);
    if (TZX_index[TZX_actualblock].sup == FALSE) {
      AS_printf ("Nulo\n");
      continue;			//si no era valido lo ignoro.
    }
    switch (TZX_index[TZX_actualblock].id) {
    case 0x10:
      AS_printf ("Bloque Estandar\n");
      //AS_printf(("seek a: %i\n",TZX_index[TZX_actualblock].offset+3));                      
      fseek (fp, (TZX_index[TZX_actualblock].offset + 3), SEEK_SET);
      // AS_printf("seek echo.\n");
      bytes = fgetc (fp);
      bytes += fgetc (fp) * 0x100;
      AS_printf ("Cargando...  pedido 0x%04x, proporcionado 0x%04x\n",
		regs->DE.W, bytes - 2);

      where = regs->IX.W;
      flags = fgetc (fp);	/* read flag type and ignore it */
      howmany = regs->DE.W;
      if (howmany == 17) {
	AS_printf ("   Cabecera, NAME:");
      }

      if (bytes - 2 < howmany) {
	howmany = bytes - 2;
	(regs->AF.B.l &= ~(C_FLAG));
	AS_printf ("Generating a tape load error (tapbytes < DE)...\n");
	regs->IX.W += bytes - 2;
      } else
	regs->IX.W += regs->DE.W;

      for (f = 0; f < howmany; f++) {
	      z80_poke(where + f, fgetc (fp));
	      if (howmany == 17 && f <= 10)
	        putchar (z80_peek(where + f));
        }
      if (howmany == 17) {
	        AS_printf ("\n");
      }

      fgetc (fp);		/* read checksum (and ignore it :-) */

      /* if load was successful, reset C_FLAG */
      if (howmany == regs->DE.W)
	       regs->AF.B.l |= (C_FLAG);

      regs->DE.W = 0;
      enviado = TRUE;
      TZX_actualblock++;	// as I exit of the loop I need to use this.
      break;
    case 0x32:
      AS_printf ("Bloque de ");
      fseek (fp, (TZX_index[TZX_actualblock].offset + 3), SEEK_SET);
      howmany = fgetc (fp);
      AS_printf (" %i Informaciones:\n\n", howmany);
      for (; howmany > 0; howmany--) {
	where = fgetc (fp);
	switch (where) {
	case 0x00:
	  AS_printf ("Titulo....: ");
	  break;
	case 0x01:
	  AS_printf ("Compania..: ");
	  break;
	case 0x02:
	  AS_printf ("Autores...: ");
	  break;
	case 0x03:
	  AS_printf ("Ano.......: ");
	  break;
	case 0x04:
	  AS_printf ("Idioma....: ");
	  break;
	case 0x05:
	  AS_printf ("Tipo......: ");
	  break;
	case 0x06:
	  AS_printf ("Precio....: ");
	  break;
	case 0x07:
	  AS_printf ("Proteccion: ");
	  break;
	case 0x08:
	  AS_printf ("Origen....: ");
	  break;
	case 0xFF:
	  AS_printf ("Comentario: ");
	  break;
	}
	where = fgetc (fp);
	for (; where > 0; where--) {
	  AS_printf ("%c", fgetc (fp));
	}
	AS_printf ("\n");
      }
      AS_printf ("\n");
      break;
    default:
      AS_printf ("Aun no soportado\n");
      break;
    }
    if (enviado == TRUE)
      break;			// si ya se paso el bloque se acabo.
  }
  return 0;
}

/* Rebobina una Cinta .TZX */
uint8_t TZX_rewind (){
  TZX_actualblock = 0;
  return 0;
}

/* genera indices de bloques del archivo .TZX */
uint8_t TZX_genindex (FILE * fp){
/* ?por que genero un indice de los TZX */

/* para empezar por culpa de los bloques de salto, call y loop
   me ahorran el releer todo el tzx cada vez que encuentro uno
   ademas me permiten comprobar de una tacada que estoy leyendo
   bien la estructura de los tzx y que no me equivoco */

/* informacion de los bloques

Los bloques no validos son ignorados directamente, los validos, son tratados
por load_block (incluso aunque no tengan efecto).	
	
valido id(HEX) descripcion tama�o sin contar el ID
(NO valido significa que no cargara correctamente en aspectrum, Valido significa que PUEDE cargar)
*	SI 10 standar tap block (2,3)+4
*	NO 11 turbo tap block  (f,10,11)+12
*	NO 12 tono 4
*	NO 13 pulsos 0*2+1
*	NO 14 pure data (7,8,9)+10
*	NO 15 voc data (5,6,7)+8
*	NO 16 c64 data standar (0,1,2,3)
*	NO 17 c64 data turbo (0,1,2,3)
*	NO 20 pause/stop tape 2
*	SI 21 group start (0)
*	SI 22 group end 0
*	SI 23 jump 2
*	SI 24 loop 2
*	SI 25 loop end 0
*	SI 26 call (0,1)*2+2
*	SI 27 return 0
*	SI 28 selection block (0,1)+2  *** no comprendo para que es esto ***
*	NO 2a stop if in 48k 4
*	SI 30 texto (0)+1
*	SI 31 mensaje (1)+2
*	SI 32 informacion (0,1)+2
*	SI 33 hardware type (0)*3+1
*	NO 34 emulator info 8
*	SI 35 custon block (10,11,12,13)+14
*	SI 40 snapshot (1,2,4)+4
*	NO 5a header 9
*	NO ?? unsoported (0,1,2,3)
*/
  int size, offset, c, valido = 0, funciona = 1;
  TZX_numofblocks = 0;
  TZX_index_nblock = 256;	/* Max blocks per file = 65536 */


  // creamos espacio en memoria para el indice
  TZX_index =
    (struct TZX_block_struct *) malloc ((sizeof (struct TZX_block_struct)) *
					TZX_index_nblock);

  //nos vamos al principio        
  fseek (fp, 0, SEEK_SET);
  offset = 0;
  AS_printf ("Iniciando la Generacion del Indice del fichero TZX\n");
  // leemos el indice y tratamos el bloque hasta que llegemos al final.
  if ((c = getc (fp)) != EOF)
    do {
      TZX_index[TZX_numofblocks].id = c;
      TZX_index[TZX_numofblocks].offset = offset;
      switch (c) {
      case 0x10:		//OK 
	TZX_index[TZX_numofblocks].sup = TRUE;
	fseek (fp, 2, SEEK_CUR);
	size = fgetc (fp);
	size = size + fgetc (fp) * 0x100;
	fseek (fp, size, SEEK_CUR);
	size += 4;
	AS_printf ("  Bloque 0x10, offset: %i, size %i, Standar Data \n",
		  offset, size);
	offset += size;
	valido = 1;
	break;

      case 0x11:		// REV
	TZX_index[TZX_numofblocks].sup = FALSE;
	fseek (fp, 0x0F, SEEK_CUR);
	size = fgetc (fp);
	size = size + fgetc (fp) * 0x100;
	size = size + fgetc (fp) * 0x10000;
	fseek (fp, size, SEEK_CUR);
	size += 18;
	AS_printf ("  Bloque 0x11, offset: %i, size %i, Turbo Data\n", offset,
		  size);
	offset += size;
	funciona = 0;
	break;

      case 0x12:		// OK
	TZX_index[TZX_numofblocks].sup = FALSE;
	fseek (fp, 4, SEEK_CUR);
	AS_printf ("  Bloque 0x12, offset: %i, size 4, Tono Puro\n", offset);
	offset += 4;
	funciona = 0;
	break;

      case 0x13:		// OK
	TZX_index[TZX_numofblocks].sup = FALSE;
	size = (fgetc (fp) * 2);
	fseek (fp, size, SEEK_CUR);
	size += 1;
	AS_printf ("  Bloque 0x13, offset: %i, size %i, Pulsos\n", offset,
		  size);
	offset += size;
	funciona = 0;
	break;

      case 0x14:		// OK
	TZX_index[TZX_numofblocks].sup = FALSE;
	fseek (fp, 0x07, SEEK_CUR);
	size = fgetc (fp);
	size = size + fgetc (fp) * 0x100;
	size = size + fgetc (fp) * 0x10000;
	fseek (fp, size, SEEK_CUR);
	size += 10;
	AS_printf ("  Bloque 0x14, offset: %i, size %i, RAW square data\n",
		  offset, size);
	offset += size;
	funciona = 0;
	break;

      case 0x15:		// REV
	TZX_index[TZX_numofblocks].sup = FALSE;
	fseek (fp, 0x05, SEEK_CUR);
	size = fgetc (fp);
	size = size + fgetc (fp) * 0x100;
	size = size + fgetc (fp) * 0x10000;
	fseek (fp, size, SEEK_CUR);
	size += 8;
	AS_printf ("  Bloque 0x15, offset: %i, size %i, .VOC data\n", offset,
		  size);
	offset += size;
	funciona = 0;
	break;

      case 0x20:		// OK
	TZX_index[TZX_numofblocks].sup = FALSE;
	fseek (fp, 2, SEEK_CUR);
	AS_printf ("  Bloque 0x20, offset: %i, size 2, Pause/stop tape\n",
		  offset);
	offset += 2;
	break;

      case 0x21:		// OK 
	TZX_index[TZX_numofblocks].sup = TRUE;
	size = fgetc (fp);
	fseek (fp, size, SEEK_CUR);
	size += 1;
	AS_printf ("  Bloque 0x21, offset: %i, size %i, Group start\n", offset,
		  size);
	offset += size;
	break;

      case 0x22:		// OK
	TZX_index[TZX_numofblocks].sup = TRUE;
	AS_printf ("  Bloque 0x22, offset: %i, size 0, Group end\n", offset);
	offset += 0;
	break;

      case 0x23:		// REV
	TZX_index[TZX_numofblocks].sup = TRUE;
	fseek (fp, 2, SEEK_CUR);
	AS_printf ("  Bloque 0x23, offset: %i, size 2, Jump block\n", offset);
	offset += 2;
	break;

      case 0x24:		// OK
	TZX_index[TZX_numofblocks].sup = TRUE;
	fseek (fp, 2, SEEK_CUR);
	AS_printf ("  Bloque 0x24, offset: %i, size 2, loop block\n", offset);
	offset += 2;
	break;

      case 0x25:		// ok
	TZX_index[TZX_numofblocks].sup = TRUE;
	AS_printf ("  Bloque 0x25, offset: %i, size 0, loop end\n", offset);
	offset += 0;
	break;

      case 0x26:		// REV
	TZX_index[TZX_numofblocks].sup = TRUE;
	size = fgetc (fp);
	size = (size + fgetc (fp) * 0x100) * 2;
	fseek (fp, size, SEEK_CUR);
	size += 2;
	AS_printf ("  Bloque 0x26, offset: %i, size %i, call block\n", offset,
		  size);
	offset += size;
	break;


      case 0x27:		// REV
	TZX_index[TZX_numofblocks].sup = TRUE;
	AS_printf ("  Bloque 0x27, offset: %i, size 0, Return\n", offset);
	offset += 0;
	break;

      case 0x28:		// REV
	TZX_index[TZX_numofblocks].sup = TRUE;
	size = fgetc (fp);
	size = size + fgetc (fp) * 0x100;
	fseek (fp, size, SEEK_CUR);
	size += 2;
	AS_printf ("  Bloque 0x28, offset: %i, size %i, selection block\n",
		  offset, size);
	offset += size;
	break;

      case 0x2A:		// REV
	TZX_index[TZX_numofblocks].sup = FALSE;
	fseek (fp, 4, SEEK_CUR);
	AS_printf ("  Bloque 0x2A, offset: %i, size 4, Stop Tape if in 48K\n",
		  offset);
	offset += 4;
	break;

      case 0x30:		// OK
	TZX_index[TZX_numofblocks].sup = TRUE;
	size = fgetc (fp);
	fseek (fp, size, SEEK_CUR);
	size += 1;
	AS_printf ("  Bloque 0x%02X, offset: %i, size %i, texto\n", c, offset,
		  size);
	offset += size;
	break;

      case 0x31:		// REV
	TZX_index[TZX_numofblocks].sup = TRUE;
	fseek (fp, 1, SEEK_CUR);
	size = fgetc (fp);
	fseek (fp, size, SEEK_CUR);
	size += 2;
	AS_printf ("  Bloque 0x%02X, offset: %i, size %i, mensaje\n", c,
		  offset, size);
	offset += size;
	break;

      case 0x32:		// OK
	TZX_index[TZX_numofblocks].sup = TRUE;
	size = fgetc (fp);
	size = size + fgetc (fp) * 0x100;
	fseek (fp, size, SEEK_CUR);
	size += 2;
	AS_printf ("  Bloque 0x%02X, offset: %i, size %i, informacion\n", c,
		  offset, size);
	offset += size;
	break;

      case 0x33:		// REV
	TZX_index[TZX_numofblocks].sup = TRUE;
	size = fgetc (fp) * 3;
	fseek (fp, size, SEEK_CUR);
	size += 1;
	AS_printf ("  Bloque 0x%02X, offset: %i, size %i, hardware type\n", c,
		  offset, size);
	offset += size;
	break;


      case 0x34:		// REV
	TZX_index[TZX_numofblocks].sup = FALSE;
	fseek (fp, 8, SEEK_CUR);
	AS_printf ("  Bloque 0x34, offset: %i, size 8, Emul. Info.\n", offset);
	offset += 8;
	break;

      case 0x35:		// REV
	TZX_index[TZX_numofblocks].sup = TRUE;
	fseek (fp, 0x0A, SEEK_CUR);
	size = fgetc (fp);
	size = size + fgetc (fp) * 0x100;
	size = size + fgetc (fp) * 0x10000;
	size = size + fgetc (fp) * 0x1000000;
	fseek (fp, size, SEEK_CUR);
	size += 14;
	AS_printf ("  Bloque 0x%02X, offset: %i, size %i, custom block\n", c,
		  offset, size);
	offset += size;
	break;

      case 0x40:		// REV
	TZX_index[TZX_numofblocks].sup = TRUE;
	fseek (fp, 0x01, SEEK_CUR);
	size = fgetc (fp);
	size = size + fgetc (fp) * 0x100;
	size = size + fgetc (fp) * 0x10000;
	fseek (fp, size, SEEK_CUR);
	size += 4;
	AS_printf ("  Bloque 0x%02X, offset: %i, size %i, snapshot\n", c,
		  offset, size);
	offset += size;
	break;

      case 0x5a:		// OK
	TZX_index[TZX_numofblocks].sup = FALSE;
	fseek (fp, 9, SEEK_CUR);
	AS_printf ("  Bloque 0x%02X, offset: %i, size 9, Cabecera\n", c,
		  offset);
	offset += 9;
	break;

      case 0x16:		// REV
      case 0x17:
      default:
	TZX_index[TZX_numofblocks].sup = FALSE;
	size = fgetc (fp);
	size = size + fgetc (fp) * 0x100;
	size = size + fgetc (fp) * 0x10000;
	size = size + fgetc (fp) * 0x1000000;
	fseek (fp, size, SEEK_CUR);
	size += 4;
	AS_printf
	  ("  Bloque 0x%02X, offset: %i, size %i, Desconocido o no soportado\n",
	   c, offset, size);
	offset += size;
	break;

      }
      TZX_numofblocks++;
      offset++;
    } while ((c = fgetc (fp)) != EOF);
  AS_printf ("indice completo %i bloques,en offset: %i, %s es valido, %s.\n",
	    TZX_numofblocks, offset, (valido == 1 ? "" : "no"),
	    (funciona == 1 ? "funcionara" : "no funcionara"));
  return valido;
}

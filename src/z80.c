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
 * see z80.h for  more info.
 *======================================================================
 *
 * This file contains the pure Z80 microprocesor emulation
 *
 *======================================================================
 */
#include <stdint.h>
#include <stdio.h>
#include "spectrum.h"
#include "tables.h"
#include "z80.h"
#include "macros.h"

/* RAM variable, debug toggle variable, pressed key and
   row variables for keyboard emulation                   */
//extern byte *RAM;
//extern int debug, main_tecla, scanl;
//extern char *msg_log;
//extern int fila[5][5];
//extern char *tapfile;
//extern FILE *tapfile;
//extern char *tfont;
//#include "macros.c"           // now in aspec.h
//extern int TSTATES_PER_LINE, TOP_BORDER_LINES, BOTTOM_BORDER_LINES, SCANLINES;
// previously in macros.c

/* Whether a half carry occured or not can be determined by looking at
   the 3rd bit of the two arguments and the result; these are hashed
   into this table in the form r12, where r is the 3rd bit of the
   result, 1 is the 3rd bit of the 1st argument and 2 is the
   third bit of the 2nd argument; the tables differ for add and subtract
   operations */
byte halfcarry_add_table[] = { 0, FLAG_H, FLAG_H, FLAG_H, 0, 0, 0, FLAG_H };
byte halfcarry_sub_table[] = { 0, 0, FLAG_H, 0, FLAG_H, 0, FLAG_H, FLAG_H };

/* Similarly, overflow can be determined by looking at the 7th bits; again
   the hash into this table is r12 */
byte overflow_add_table[] = { 0, 0, 0, FLAG_V, FLAG_V, 0, 0, 0 };
byte overflow_sub_table[] = { 0, FLAG_V, 0, 0, 0, 0, FLAG_V, 0 };

/* Some more tables; initialised in z80_init_tables() */
byte sz53_table[0x100];    /* The S, Z, 5 and 3 bits of the temp value */
byte parity_table[0x100]; /* The parity of the temp value */
byte sz53p_table[0x100];  /* OR the above two tables together */
/*------------------------------------------------------------------*/

// Contributed by Metalbrain to implement OUTI, etc.
byte ioblock_inc1_table[64];
byte ioblock_dec1_table[64];
byte ioblock_2_table[0x100];

/*====================================================================
  void Z80Reset( Z80Regs *regs, int cycles )

  This function simulates a z80 reset by setting the registers
  to the values they are supposed to take on a real z80 reset.
  You must pass it the Z80 register structure and the number
  of cycles required to check for interrupts and do special
  hardware checking/updating.
 ===================================================================*/
void Z80Reset (Z80Regs * regs, int int_cycles) {
  /* reset PC and the rest of main registers: */
  regs->PC.W = regs->R.W = 0x0000;
  regs->AF.W = regs->BC.W = regs->DE.W = regs->HL.W = 0x0000;
  regs->AFs.W = regs->BCs.W = regs->DEs.W = regs->HLs.W = 0x0000;
  regs->IX.W = regs->IY.W = 0x0000;
  /* Make the stack point to $F000 */
  regs->SP.W = 0xF000;
  /* reset variables to their default values */
  regs->I = 0x00;
  regs->IFF1 = regs->IFF2 = regs->IM = regs->halted = 0x00;
  regs->ICount = regs->IPeriod = int_cycles;
  regs->IRequest = INT_NOINT;
  regs->we_are_on_ddfd = regs->dobreak = 0;
  //regs->BorderColor = 0;
}

/*====================================================================
  word Z80Run( Z80Regs *regs, int numopcodes )

  This function does the whole Z80 simulation. It consists on a
  for(;;) loop (as stated on Marat's Fayzullin HOWTO -How to
  Write a Computer Emulator-) which fetchs the next opcode,
  interprets it (using a switch statement) and then it's
  executed in the right CASE: of that switch. I've put the different
  case statements into C files included here with #include to
  make this more readable (and programming easier! :).

  This function will change regs->ICount register and will execute
  an interrupt when it reaches 0 (or <0). You can then do anything
  related to your machine emulation here, using the Z80Hardware()
  function. This function must be filled by yourself: put there
  the code related to the emulated machine hardware, such as
  screen redrawing, sound playing and so on. This functions can
  return an special value to make Z80Run stop the emulation (and
  return to the caller): that's INT_QUIT. If there is time to
  execute an interrupt, please return INT_IRQ or INT_NMI. Return
  INT_NOINT if there is no time for an interrupt :) .

  Z80Execute() will change PC and all the z80 registers acording
  to the executed opcode, and those values will be returned when
  a INT_QUIT is received.

  Pass as numcycles the number of clock cycle you want to execute
  z80 opcodes for or < 0 (negative) to execute "infinite" opcodes.
 ===================================================================*/
uint16_t Z80Run (Z80Regs * regs, int numcycles) {
  /* opcode and temp variables */
  register byte opcode;
  eword tmpreg, ops, mread, tmpreg2;
  unsigned long tempdword;
  register int loop;
  unsigned short tempword;

  /* emulate <numcycles> cycles */
  loop = (regs->ICount - numcycles);
  /* this is the emulation main loop */
  while (regs->ICount > loop) {
      if (regs->halted == 1) {
       r_PC--;
       AddCycles (4);
      }
      /* read the opcode from memory (pointed by PC) */
      opcode = Z80ReadMem (regs->PC.W);
      regs->PC.W++;
      /* increment the R register and decode the instruction */
      AddR (1);
      switch (opcode)  {
      #include "opcodes.h"
      case PREFIX_CB:
        AddR (1);
        #include "op_cb.h"
        break;
      case PREFIX_ED:
        AddR (1);
        #include "op_ed.h"
        break;
      case PREFIX_DD:
        AddR (1);
        regs->we_are_on_ddfd = WE_ARE_ON_DD;
        #define REGISTER regs->IX
        #include "op_dd_fd.h"
        #undef  REGISTER
        regs->we_are_on_ddfd = 0;
        break;
      case PREFIX_FD:
        AddR (1);
        regs->we_are_on_ddfd = WE_ARE_ON_FD;
        #define REGISTER regs->IY
        #include "op_dd_fd.h"
        #undef  REGISTER
        regs->we_are_on_ddfd = 0;
        break;
      }

      /* patch ROM loading routine */
      // address contributed by Ignacio Burgueño :)
      if (r_PC >= 0x0556 && r_PC <= 0x056c) Z80Patch (regs);

      /* check if it's time to do other hardware emulation */
      if (regs->ICount <= 0) {
         regs->petint=0;
         regs->ICount += regs->IPeriod;
         loop = regs->ICount + loop;
         Z80Interrupt (regs, tmpreg.W);
      }
  }
  return (regs->PC.W);
}



/*====================================================================
  void Z80Interrupt( Z80Regs *regs, word ivec )
 ===================================================================*/
void Z80Interrupt (Z80Regs * regs, uint16_t ivec){
  uint16_t intaddress;

  /* unhalt the computer */
  if (regs->halted == 1)
    regs->halted = 0;

  if (regs->IFF1)
    {
      PUSH (PC);
      regs->IFF1 = 0;
      switch (regs->IM) {
       case 0:
        r_PC = 0x0038;
        AddCycles (12);
        break;
       case 1:
        r_PC = 0x0038;
        AddCycles (13);
        break;
       case 2:
        intaddress = (((regs->I & 0xFF) << 8) | 0xFF);
        regs->PC.B.l = Z80ReadMem (intaddress);
        regs->PC.B.h = Z80ReadMem (intaddress + 1);
        AddCycles (19);
        break;
       }
    }
}


/*====================================================================
  word  Z80Hardware(register Z80Regs *regs)

  Do here your emulated machine hardware emulation. Read Z80Execute()
  to know about how to quit emulation and generate interrupts.
 ===================================================================*/
uint16_t Z80Hardware (register Z80Regs * regs){
  return (INT_IRQ);
}


/*====================================================================
  void Z80Patch( register Z80Regs *regs )

  Write here your patches to some z80 opcodes that are quite related
  to the emulated machines (i.e. maybe accessing to the I/O ports
  and so on), such as ED_FE opcode:

     case ED_FE:     Z80Patch(regs);
                     break;

  This allows "BIOS" patching (cassette loading, keyboard ...).
 ===================================================================*/

void
Z80Patch (register Z80Regs * regs)
{
// QUE ALGUIEN ME EXPLIQUE por que hay dos tapfile ???
/*  extern FILE *tapfile;  
  extern tipo_emuopt emuopt;
    if (emuopt.tapefile[0] != 0)
    {
    //  AS_printf("Z80patch:%x\n",tapfile);
      LoadTAP (regs, tapfile);
      POP (PC);
    }
*/
}





/*====================================================================
  byte Z80MemRead( register word address )

  This function reads from the given memory address. It is not inlined,
  and it's written for debugging purposes.
 ===================================================================*/
byte
Z80MemRead (register uint16_t address, Z80Regs * regs)
{
  return (z80_peek(address));
}

/*====================================================================
  void Z80MemWrite( register word address, register byte value )

  This function writes on memory the given value. It is not inlined,
  ands it's written for debugging purposes.
 ===================================================================*/
void Z80MemWrite (register uint16_t address, register byte value, Z80Regs * regs) {
  z80_poke(address,value);
}

/*--- Memory Write on the A address on no bank machines -------------*/
void Z80WriteMem (uint16_t where, uint16_t A, Z80Regs * regs){
  z80_poke(where, (byte)A);
}

byte Z80ReadMem (uint16_t where){
  return (z80_peek(where));
}

/*====================================================================
  byte Z80InPort(register Z80Regs *regs, eregister word port )

  This function reads from the given I/O port. It is not inlined,
  and it's written for debugging purposes.
 ===================================================================*/
byte Z80InPort (register Z80Regs * regs, register uint16_t port) {
  return (z80_in(port));
}

/*====================================================================
  void Z80OutPort( register word port, register byte value )

  This function outs a value to a given I/O port. It is not inlined,
  and it's written for debugging purposes.
 ===================================================================*/
void Z80OutPort (register Z80Regs * regs, register uint16_t port, register byte value) {
 z80_out(port,value);
}


/*====================================================================
   static void Z80FlagTables ( void );

   Creates a look-up table for future flag setting...
   Taken from fuse's sources. Thanks to Philip Kendall.
 ===================================================================*/
void Z80FlagTables (void) {
  int i, j, k;
  byte parity;

  for (i = 0; i < 0x100; i++) {
      sz53_table[i] = i & (FLAG_3 | FLAG_5 | FLAG_S);
      j = i;
      parity = 0;
      for (k = 0; k < 8; k++)   {
        parity ^= j & 1;
        j >>= 1;
      }
      parity_table[i] = (parity ? 0 : FLAG_P);
      sz53p_table[i] = sz53_table[i] | parity_table[i];
  }
  sz53_table[0] |= FLAG_Z;
  sz53p_table[0] |= FLAG_Z;
}
 

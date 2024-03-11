#ifndef SPECTRUM_H
#define SPECTRUM_H

#include "z80.h"

enum models_enum { SPECMDL_16K=1, SPECMDL_48K, SPECMDL_INVES, SPECMDL_128K, SPECMDL_PLUS2, 
       SPECMDL_PLUS3, SPECMDL_48KIF1, SPECMDL_48KTRANSTAPE };
enum inttypes_enum { NORMAL=1, INVES }; 

#define RO_PAGE 1
#define RW_PAGE 0
#define SYSTEM_PAGE 0
#define NOSYS_PAGE 1

typedef struct {  
  int ts_lebo;      // left border t states
  int ts_grap;      // graphic zone t states
  int ts_ribo;      // right border t states
  int ts_hore;      // horizontal retrace t states
  int ts_line;      // to speed the calc, the sum of 4 abobe
  int line_poin;    // lines in retraze post interrup
  int line_upbo;    // lines of upper border
  int line_grap;    // lines of graphic zone = 192
  int line_bobo;    // lines of bottom border
  int line_retr;    // lines of the retrace  
  int TSTATES_PER_LINE;
  int TOP_BORDER_LINES;
  int SCANLINES;
  int BOTTOM_BORDER_LINES;
  int tstate_border_left;
  int tstate_graphic_zone;
  int tstate_border_right;
  int hw_model;
  int int_type;
  int videopage;
  int BANKM;
  int BANK678;
  int emulate_FF;  
  uint8_t BorderColor; 
  uint8_t portFF; 
  uint8_t SoundBits; 
} tipo_hwopt ;

typedef struct  { //estructura de memoria
   uint8_t *p;     // pointer to memory poll
   int mp;      // bitmap for page bits in z80 mem
   int md;      // bitmap for dir bits in z80 mem
   int np;      // number of pages of memory 
   int ro[16];  // map of pages for read  (map ar alwais a offset for *p) 
   int wo[16];  // map of pages for write
   int sro[16]; // map of system memory pages for read (to remember when perife-
   int swo[16]; // map of system memory pages for write         -rical is paged)
   int vn;      // number of video pages
   int vo[2];   // map of video memory
   int roo;     // offset to dummy page for readonly emulation
             //   Precalculated data
   int mr;   // times left rotations for page ( number of zero of mp)
   int sp;   // size of pages (hFFFF / mp)
} tipo_mem;



// de Aspectrum
//extern uint8_t z80_mem[];
uint8_t z80_peek(uint16_t dir);
void    z80_poke(uint16_t dir, uint8_t dato);
uint8_t readvmem(uint16_t offset, int page);
void pagein(int size, int bloq, int page, int ro, int issystem);
void pageout(int size, int bloq, int page);

uint8_t z80_in(uint16_t  dir);
void    z80_out(uint16_t dir,uint8_t dato);

int init_spectrum(int model,const char *romfile);
int end_spectrum(void);
int reset_spectrum(Z80Regs *);

int init_48k(const char *romfile);
int init_16k(const char *romfile);
int init_inves(const char *romfile);

int init_plus2(void);
int init_128k(void);
int reset_128k(void);
void outbankm_128k(uint8_t dato);

int init_plus3(void);
int reset_plus3(void);
void outbankm_p31(uint8_t dato);
void outbankm_p37(uint8_t dato);

// esto esta en main.ino
int load_48krom(const char *);
int load_128krom(const char *);
int load_p3rom(const char *);

#endif  // #ifdef SPECTRUM_H

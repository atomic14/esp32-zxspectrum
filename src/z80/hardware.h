#ifndef HARDWARE_H
#define HARDWARE_H
// #ifdef __cplusplus
// extern "C"
// {
// #endif

#include <stdint.h>
#include "spectrum.h"

  class TFT_eSPI;

#define KEYPADADRESS 0x20

#define KEYCFG_1 "O,P,Q,A y Space"
#define KEYCFG_2 "Joysktick Kempston"
#define KEYCFG_3 "Joystick Sinclair"
#define KEYCFG_4 "P,L,Z,X y M"

  enum SpecKeys
  {
    SPECKEY_NONE,
    SPECKEY_1,
    SPECKEY_2,
    SPECKEY_3,
    SPECKEY_4,
    SPECKEY_5,
    SPECKEY_6,
    SPECKEY_7,
    SPECKEY_8,
    SPECKEY_9,
    SPECKEY_0,
    SPECKEY_Q,
    SPECKEY_W,
    SPECKEY_E,
    SPECKEY_R,
    SPECKEY_T,
    SPECKEY_Y,
    SPECKEY_U,
    SPECKEY_I,
    SPECKEY_O,
    SPECKEY_P,
    SPECKEY_A,
    SPECKEY_S,
    SPECKEY_D,
    SPECKEY_F,
    SPECKEY_G,
    SPECKEY_H,
    SPECKEY_J,
    SPECKEY_K,
    SPECKEY_L,
    SPECKEY_ENTER,
    SPECKEY_SHIFT,
    SPECKEY_Z,
    SPECKEY_X,
    SPECKEY_C,
    SPECKEY_V,
    SPECKEY_B,
    SPECKEY_N,
    SPECKEY_M,
    SPECKEY_SYMB,
    SPECKEY_SPACE,
    JOYK_UP,
    JOYK_DOWN,
    JOYK_LEFT,
    JOYK_RIGHT,
    JOYK_FIRE,
    VEGAKEY_MENU
  };

  typedef struct
  {
    //  char romfile[256];
    //  char snapfile[256];
    char tapefile[256];
    uint8_t mappingindex = 0;
    bool sonido = 0;
  } tipo_emuopt;

  // extern uint8_t border;
  extern uint8_t kempston_port;
  extern uint8_t ulaport_FF;
  extern uint8_t speckey[];

  extern int keys[];
  extern int oldkeys[];

  void updatekey(uint8_t key, uint8_t state);

// #ifdef __cplusplus
// }
// #endif
#endif // #ifdef HARDWARE_H

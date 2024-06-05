#pragma once

#include <unordered_map>

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
};

const std::unordered_map<SpecKeys, char> specKeyToLetter = {
  { SPECKEY_0, '0' },
  { SPECKEY_1, '1' },
  { SPECKEY_2, '2' },
  { SPECKEY_3, '3' },
  { SPECKEY_4, '4' },
  { SPECKEY_5, '5' },
  { SPECKEY_6, '6' },
  { SPECKEY_7, '7' },
  { SPECKEY_8, '8' },
  { SPECKEY_9, '9' },
  { SPECKEY_A, 'A' },
  { SPECKEY_B, 'B' },
  { SPECKEY_C, 'C' },
  { SPECKEY_D, 'D' },
  { SPECKEY_E, 'E' },
  { SPECKEY_F, 'F' },
  { SPECKEY_G, 'G' },
  { SPECKEY_H, 'H' },
  { SPECKEY_I, 'I' },
  { SPECKEY_J, 'J' },
  { SPECKEY_K, 'K' },
  { SPECKEY_L, 'L' },
  { SPECKEY_M, 'M' },
  { SPECKEY_N, 'N' },
  { SPECKEY_O, 'O' },
  { SPECKEY_P, 'P' },
  { SPECKEY_Q, 'Q' },
  { SPECKEY_R, 'R' },
  { SPECKEY_S, 'S' },
  { SPECKEY_T, 'T' },
  { SPECKEY_U, 'U' },
  { SPECKEY_V, 'V' },
  { SPECKEY_W, 'W' },
  { SPECKEY_X, 'X' },
  { SPECKEY_Y, 'Y' },
  { SPECKEY_Z, 'Z' },
};
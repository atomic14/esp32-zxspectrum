#pragma once

#include "Files.h"

class Flash : public Files
{
public:
  Flash();
  ~Flash();
  bool isMounted();
};
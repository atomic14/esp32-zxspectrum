#pragma once

#include <string>
#include <vector>
#include "spectrum.h"

void loadGame(const std::string& filename, ZXSpectrum* machine);
void loadGame(uint8_t* data, size_t length, const std::string& filename, ZXSpectrum* machine);
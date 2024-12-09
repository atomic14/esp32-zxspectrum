#include <SDL.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif
#include "loadgame.h"
#include <iostream>
#include <cstdint>
#include <unordered_map>
#include "spectrum.h"
#include "tzx_cas.h"
#include "snaps.h"
#include "RawAudioListener.h"
#include "DummyListener.h"
#include "ZXSpectrumTapeListener.h"
#include <fstream>
#include <vector>

void loadGame(const std::string& filename, ZXSpectrum* machine) {
    // Read file into buffer
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return;
    }
    
    std::vector<uint8_t> buffer(std::istreambuf_iterator<char>(file), {});
    loadGame(buffer.data(), buffer.size(), filename, machine);
}

void loadGame(uint8_t* tzx_data, size_t file_size, const std::string& filename, ZXSpectrum* machine) {
    // time how long it takes to load the tape
    int start = SDL_GetTicks();
    // load the tape
    TzxCas tzxCas;
    DummyListener *dummyListener = new DummyListener();
    dummyListener->start();
    tzxCas.load_tzx(dummyListener, tzx_data, file_size);
    dummyListener->finish();
    uint64_t totalTicks = dummyListener->getTotalTicks();

    ZXSpectrumTapeListener *listener = new ZXSpectrumTapeListener(machine, [&](uint64_t progress)
      {
        // printf("Total execution time: %fs\n", (float) listener->getTotalExecutionTime() / 1000000.0f);
        // printf("Total machine time: %f\n", (float) listener->getTotalTicks() / 3500000.0f);
        // printf("Progress: %lld\n", progress * 100 / totalTicks);
      });
    listener->start();
    if (filename.find(".tap") != std::string::npos || filename.find(".TAP") != std::string::npos) {
        tzxCas.load_tap(listener, tzx_data, file_size);
    } else {
        tzxCas.load_tzx(listener, tzx_data, file_size);
    }
    listener->finish();
    std::cout << "Total ticks: " << listener->getTotalTicks() << std::endl;
    std::cout << "Total time: " << listener->getTotalTicks() / 3500000.0 << " seconds" << std::endl;

    std::cout << "Loaded tape." << std::endl;
    int end = SDL_GetTicks();
    std::cout << "Time to load tape: " << end - start << "ms" << std::endl;
}
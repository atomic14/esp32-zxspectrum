#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/bind.h>
#include <emscripten/val.h>
#endif
#include <iostream>
#include <cstdint>
#include "spectrum.h"
#include "snaps.h"
#include "loadgame.h"


Z80MemoryWriter *write(std::string filename, uint8_t *data, size_t length, bool is128k) {
    bool isTAP = filename.find(".tap") != std::string::npos || filename.find(".TAP") != std::string::npos;
    bool isTZX = filename.find(".tzx") != std::string::npos || filename.find(".TZX") != std::string::npos;
    if (!isTAP && !isTZX) {
        std::cerr << "Unsupported file format. Only .tap and .tzx files are supported." << std::endl;
        return nullptr;
    }
    ZXSpectrum *machine = new ZXSpectrum();
    machine->reset();
    if (is128k) {
        std::cout << "Using 128k machine" << std::endl;
        machine->init_spectrum(SPECMDL_128K);
        machine->reset_spectrum(machine->z80Regs);
        for(int i = 0; i < 200; i++) {
            machine->runForFrame(nullptr, nullptr);
        }
    } else {
        std::cout << "Using 48k machine" << std::endl;
        machine->init_spectrum(SPECMDL_48K);
        machine->reset_spectrum(machine->z80Regs);
        for(int i = 0; i < 200; i++) {
            machine->runForFrame(nullptr, nullptr);
        }
        // we need to do load ""
        machine->updateKey(SPECKEY_J, 1);
        for(int i = 0; i < 10; i++) {
            machine->runForFrame(nullptr, nullptr);
        }
        machine->updateKey(SPECKEY_J, 0);
        for(int i = 0; i < 10; i++) {
            machine->runForFrame(nullptr, nullptr);
        }
        machine->updateKey(SPECKEY_SYMB, 1);
        for(int i = 0; i < 10; i++) {
            machine->runForFrame(nullptr, nullptr);
        }
        machine->updateKey(SPECKEY_P, 1);
        for(int i = 0; i < 10; i++) {
            machine->runForFrame(nullptr, nullptr);
        }
        machine->updateKey(SPECKEY_P, 0);
        for(int i = 0; i < 10; i++) {
            machine->runForFrame(nullptr, nullptr);
        }
        machine->updateKey(SPECKEY_P, 1);
        for(int i = 0; i < 10; i++) {
            machine->runForFrame(nullptr, nullptr);
        }
        machine->updateKey(SPECKEY_P, 0);
        for(int i = 0; i < 10; i++) {
            machine->runForFrame(nullptr, nullptr);
        }
        machine->updateKey(SPECKEY_SYMB, 0);
    }
    // press enter
    machine->updateKey(SPECKEY_ENTER, 1);
    for(int i = 0; i < 10; i++) {
        machine->runForFrame(nullptr, nullptr);
    }
    machine->updateKey(SPECKEY_ENTER, 0);
    for(int i = 0; i < 10; i++) {
        machine->runForFrame(nullptr, nullptr);
    }
    loadTapeGame(data, length, filename, machine);
    Z80MemoryWriter *writer = new Z80MemoryWriter(machine);
    writer->saveZ80();
    delete machine;
    return writer;
}

#ifdef __EMSCRIPTEN__
// JavaScript interface function that takes an array and returns WAV data
emscripten::val tap_to_z80(std::string filename, const emscripten::val& arrayBuffer, bool is128k) {
    ZXSpectrum *machine = new ZXSpectrum();

    size_t length = arrayBuffer["byteLength"].as<size_t>();
    uint8_t* data = (uint8_t*)malloc(length);

    // Copy data from JS ArrayBuffer to C++ memory
    emscripten::val memoryView = emscripten::val::global("Uint8Array").new_(arrayBuffer);
    for (size_t i = 0; i < length; i++) {
        data[i] = memoryView[i].as<uint8_t>();
    }

    Z80MemoryWriter *writer = write(filename, data, length, is128k);
    auto result = emscripten::val(emscripten::typed_memory_view(
        writer->size(), 
        writer->data()
    ));
    free(data);
    delete writer;
    return result;
}

// Bind the JavaScript interface
EMSCRIPTEN_BINDINGS(tape_module) {
    emscripten::function("convertTapeToZ80", &tap_to_z80);
}
#endif

#ifndef __EMSCRIPTEN__
// test main file - takes a tap file as the first argument a machine type for the second argument (48k or 128k) and writes a z80 version
int main(int argc, char *argv[])
{
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <filename> <machine_type>" << std::endl;
        return 1;
    }
    std::string filename = argv[1];
    bool is128k = false;
    if (strcmp(argv[2], "128k") == 0) {
        is128k = true;
    }
    FILE *file = fopen(filename.c_str(), "rb");
    if (!file) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return 1;
    }
    fseek(file, 0, SEEK_END);
    size_t length = ftell(file);
    fseek(file, 0, SEEK_SET);
    uint8_t *data = (uint8_t *)malloc(length);
    fread(data, 1, length, file);
    fclose(file);
    Z80MemoryWriter *writer = write(filename, data, length, is128k);
    free(data);
    // Save the Z80 file
    std::string z80Filename = filename.substr(0, filename.find_last_of('.')) + ".z80";
    FILE *z80File = fopen(z80Filename.c_str(), "wb");
    if (!z80File) {
        std::cerr << "Failed to open file: " << z80Filename << std::endl;
        return 1;
    }
    fwrite(writer->data(), 1, writer->size(), z80File);
    fclose(z80File);
    std::cout << "Z80 file saved as: " << z80Filename << std::endl;
    std::cout << "Z80 file size: " << writer->size() << " bytes" << std::endl;
    delete writer;
    return 0;
}
#endif

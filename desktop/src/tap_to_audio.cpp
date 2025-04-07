#include <iostream>
#include <vector>
#include <fstream>
#include <emscripten/bind.h>
#include "RawAudioListener.h"
#include "DummyListener.h"
#include "tzx_cas.h"

// Helper function that takes a buffer pointer and length, returns WAV data
std::vector<uint8_t> convert_tape_to_wav(const uint8_t* tape_data, size_t length, bool is_tap) {
    std::vector<uint8_t> wavFile;
    auto listener = new RawAudioListener(wavFile);
    TzxCas tzxCas;
    
    // Create a non-const copy of the data since tzx_cas requires non-const pointers
    std::vector<uint8_t> data_copy(tape_data, tape_data + length);
    
    listener->start();
    if (is_tap) {
        tzxCas.load_tap(listener, data_copy.data(), length);
    } else {
        tzxCas.load_tzx(listener, data_copy.data(), length);
    }
    listener->finish();
    
    delete listener;
    return wavFile;
}

// JavaScript interface function that takes an array and returns WAV data
emscripten::val convert_tape_file(emscripten::val tape_data, bool is_tap) {
    // Convert JavaScript array to C++ vector
    std::vector<uint8_t> input;
    const auto length = tape_data["length"].as<unsigned>();
    input.resize(length);
    emscripten::val memory = emscripten::val::global("Uint8Array").new_(tape_data);
    memory.call<void>("set", tape_data);
    for(unsigned i = 0; i < length; ++i) {
        input[i] = memory[i].as<uint8_t>();
    }
    
    // Convert to WAV
    auto wav_data = convert_tape_to_wav(input.data(), input.size(), is_tap);
    
    // Return WAV data as Uint8Array
    return emscripten::val(emscripten::typed_memory_view(
        wav_data.size(), 
        wav_data.data()
    ));
}

// Bind the JavaScript interface
EMSCRIPTEN_BINDINGS(tape_module) {
    emscripten::function("convertTapeFile", &convert_tape_file);
}

#ifndef EMSCRIPTEN
// Keep the main function for command line usage, but only when not compiling for web
int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        std::cerr << "Usage: " << argv[0] << " <tap file> <output file>" << std::endl;
        return 1;
    }

    std::string filename = argv[1];
    std::string outfile = argv[2];

    // Read input file
    std::ifstream file(filename, std::ios::binary);
    std::vector<uint8_t> buffer(std::istreambuf_iterator<char>(file), {});
    
    // Convert to WAV
    bool is_tap = (filename.find(".tap") != std::string::npos || 
                   filename.find(".TAP") != std::string::npos);
    auto wav_data = convert_tape_to_wav(buffer.data(), buffer.size(), is_tap);

    // Write output file
    std::ofstream outStream(outfile, std::ios::binary);
    outStream.write(reinterpret_cast<const char*>(wav_data.data()), wav_data.size());
    outStream.close();

    return 0;
}
#endif
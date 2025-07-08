# ZX Spectrum Emulator

This uses the same code as the ESP32 version, but makes it a bit easier to develop and test as we can run it on the desktop.

This is very much "dev" software, so expect bugs and missing features.

# Building for the desktop

The code is designed to run on a Mac, but it shold be pretty easy to get running on windows.

```
make -f Makefile.emu clean
make -f Makefile.emu
./zx_emulator
```

On lauch you will be prompted to select a game to load. Select a z80 or tzx/tap file.

# Using Emscripten

```
./build_emscripten.sh
```

# Run Emscripten

```
python3 -m http.server
```

Now go to http://localhost:8000/zx_emulator.html

Drag and drop a z80 or tzx/tap file into the browser window.
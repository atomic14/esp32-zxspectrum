# ZX Spectrum Emulator

This uses the same code as the ESP32 version, but makes it a bit easier to develop and test as we can run it on the desktop.

# Building for the desktop

The code is designed to run on a Mac, but it shold be pretty easy to get runningon windows.

```
make clean
make
./zx_emulator
```

On lauch you will be prompted to select a game to load. Select a z80 or tzx/tap file.

# Using Emscripten

This allows you to run the emulator in your browser.

https://emscripten.org/docs/getting_started/downloads.html

```
git clone https://github.com/emscripten-core/emsdk.git
```

```
cd emsdk

## Fetch the latest version of the emsdk (not needed the first time you clone)
git pull

## Download and install the latest SDK tools.
./emsdk install latest

## Make the "latest" SDK "active" for the current user. (writes .emscripten file)
./emsdk activate latest

## Activate PATH and other environment variables in the current terminal
source ./emsdk_env.sh
```

# Build Emscripten

```
make -f Makefile.ems
```

# Run Emscripten

```
python3 -m http.server
```

Now go to http://localhost:8000/zx_emulator.html
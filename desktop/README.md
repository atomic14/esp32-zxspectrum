# ZX Spectrum Emulator

This uses the same code as the ESP32 version, but makes it a bit easier to develop and test as we can run it on the desktop.


# Setup Emscripten

https://emscripten.org/docs/getting_started/downloads.html

```
git clone https://github.com/emscripten-core/emsdk.git
```

```
cd emsdk

# Fetch the latest version of the emsdk (not needed the first time you clone)
git pull

# Download and install the latest SDK tools.
./emsdk install latest

# Make the "latest" SDK "active" for the current user. (writes .emscripten file)
./emsdk activate latest

# Activate PATH and other environment variables in the current terminal
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
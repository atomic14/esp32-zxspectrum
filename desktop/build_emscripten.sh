#!/bin/bash

# Exit on error
set -e

echo "Setting up Emscripten..."

# Clone emsdk if it doesn't exist
if [ ! -d "emsdk" ]; then
    git clone https://github.com/emscripten-core/emsdk.git
fi

# Navigate to emsdk directory
cd emsdk

# Update emsdk to latest version
git pull

# Install latest SDK tools
./emsdk install latest

# Activate the latest SDK
./emsdk activate latest

# Add environment variables to current shell
source ./emsdk_env.sh

cd ..

make -f Makefile.ems

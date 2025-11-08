#pragma once

class EmulatorScreen;
class IFiles;
class Machine;
class NavigationStack;
class PacketHandler;
class ZXSpectrum;

// Create the Config class as a singleton, globally available from the entire application
class Config 
{
private:
    Config() = default;
    ~Config() = default;

public:
    static Config& getConfig() {
        static Config instance;
        return instance;
    }

    // Screen stack
    NavigationStack *navigationStack = nullptr;

    // Serial packets handler
    PacketHandler *packetHandler = nullptr;

    // The emulator screen
    EmulatorScreen *emulatorScreen = nullptr;
    // The emulation machine wrapper
    Machine *machine = nullptr;
    // The Z80 emulation machine
    ZXSpectrum *speccy = nullptr;

    // SDCard files
    IFiles *sdfiles = nullptr;

    // Is serial used for debugging (or file transfer)
    // True when DeZog debugger is connected and messages go to the emulator
    // False when disconnected and messages are used for file transfers
    bool m_serialDebugging = false;
};
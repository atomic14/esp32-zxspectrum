#include <SDL.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif
#ifndef __EMSCRIPTEN__
#import <Cocoa/Cocoa.h>
#endif
#include <iostream>
#include <cstdint>
#include <unordered_map>
#include "spectrum.h"
#include "tzx_cas.h"
#include "snaps.h"
#include "RawAudioListener.h"
#include "DummyListener.h"
#include "ZXSpectrumTapeListener.h"
#include "SDLAudioOutput.h"
#include "loadgame.h"
#include "input.h"
#include <thread>
#include <chrono>

bool isLoading = false;
bool isRunning = false;
uint16_t flashTimer = 0;

// this matches our TFT display - but this could be any size really
const int WIDTH = 320;  // Width of the display
const int HEIGHT = 240; // Height of the display
const float ASPECT_RATIO = ((float) WIDTH / (float) HEIGHT);
ZXSpectrum *machine = nullptr;

int count = 0;

uint16_t frameBuffer[WIDTH * HEIGHT] = {0}; // Example: initialize your 16-bit frame buffer here

SDL_Window *window = nullptr;
SDL_Renderer *renderer = nullptr;
AudioOutput *audioOutput = nullptr;
SDL_Texture *texture = nullptr;

#ifndef __EMSCRIPTEN__
std::string OpenFileDialog() {
    @autoreleasepool {
        NSOpenPanel* openPanel = [NSOpenPanel openPanel];
        [openPanel setCanChooseFiles:YES];
        [openPanel setCanChooseDirectories:NO];
        [openPanel setAllowsMultipleSelection:NO];
        
        if ([openPanel runModal] == NSModalResponseOK) {
            NSURL* fileURL = [[openPanel URLs] objectAtIndex:0];
            return std::string([[fileURL path] UTF8String]);
        }
    }
    return "";
}
#endif

void enforceAspectRatio(SDL_Window* window, int newWidth, int newHeight) {
    // Calculate the new dimensions while keeping the aspect ratio
    int adjustedWidth = newWidth;
    int adjustedHeight = static_cast<int>(newWidth / ASPECT_RATIO);

    if (adjustedHeight > newHeight) {
        adjustedHeight = newHeight;
        adjustedWidth = static_cast<int>(newHeight * ASPECT_RATIO);
    }

    SDL_SetWindowSize(window, adjustedWidth, adjustedHeight);
}

// Initializes SDL, creating a window and renderer
bool initSDL(SDL_Window *&window, SDL_Renderer *&renderer)
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0)
    {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }

    window = SDL_CreateWindow("16-Bit Frame Buffer Display",
                              SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                              WIDTH * 2, HEIGHT * 2, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!window)
    {
        std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer)
    {
        std::cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }
    return true;
}

// Creates an SDL texture for displaying the frame buffer
SDL_Texture *createTexture(SDL_Renderer *renderer)
{
    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB565,
                                             SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);
    if (!texture)
    {
        std::cerr << "Texture could not be created! SDL_Error: " << SDL_GetError() << std::endl;
    }
    return texture;
}

// Updates and renders the frame buffer to the screen
void updateAndRender(SDL_Renderer *renderer, SDL_Texture *texture, uint16_t *frameBuffer)
{
    SDL_UpdateTexture(texture, nullptr, frameBuffer, WIDTH * sizeof(uint16_t));
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, nullptr, nullptr);
    SDL_RenderPresent(renderer);
}

// Handles SDL events, including quit and keyboard input
void handleEvents(bool &isRunning)
{
    SDL_Event e;
    while (SDL_PollEvent(&e) != 0)
    {
        if (e.type == SDL_QUIT)
        {
            isRunning = false;
        }
        if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_RESIZED) {
            int newWidth = e.window.data1;
            int newHeight = e.window.data2;
            enforceAspectRatio(window, newWidth, newHeight);
        }
        if (e.type == SDL_KEYDOWN)
        {
            switch (e.key.keysym.sym)
            {
            case SDLK_ESCAPE:
                #ifndef __EMSCRIPTEN__
                if(!isLoading) {
                    std::string filename = OpenFileDialog();
                    loadGame(filename, machine);
                    return;
                }
                #endif
                break;
            default:
                break;
            }
            // handle spectrum key mapping
            auto it = sdl_to_spec.find(e.key.keysym.sym);
            if (it != sdl_to_spec.end())
            {
                machine->updateKey(it->second, 1);
            }
        }
        if (e.type == SDL_KEYUP)
        {
            auto it = sdl_to_spec.find(e.key.keysym.sym);
            if (it != sdl_to_spec.end())
            {
                machine->updateKey(it->second, 0);
            }
        }
    }
}

void fillFrameBuffer(uint16_t *pixelBuffer, uint8_t *currentScreenBuffer, uint8_t machineBorderColor)
{
    // do the border
    uint8_t borderColor = machineBorderColor & 0b00000111;
    uint16_t tftColor = specpal565[borderColor];
    // swap the byte order
    tftColor = (tftColor >> 8) | (tftColor << 8);
    const int borderWidth = (WIDTH - 256) / 2;
    const int borderHeight = (HEIGHT - 192) / 2;
    // do the border with some simple rects - no need to push pixels for a solid color
    for(int y = 0; y < borderHeight; y++) {
        for(int x = 0; x < WIDTH; x++) {
            pixelBuffer[y * WIDTH + x] = tftColor;
            pixelBuffer[(HEIGHT - 1 - y) * WIDTH + x] = tftColor;
        }
    }
    for(int x = 0; x < borderWidth; x++) {
        for(int y = 0; y < HEIGHT; y++) {
            pixelBuffer[y * WIDTH + x] = tftColor;
            pixelBuffer[y * WIDTH + (WIDTH - 1 - x)] = tftColor;
        }
    }
    // do the pixels
    uint8_t *attrBase = currentScreenBuffer + 0x1800;
    uint8_t *pixelBase = currentScreenBuffer;
    for (int attrY = 0; attrY < 192 / 8; attrY++)
    {
        for (int attrX = 0; attrX < 256 / 8; attrX++)
        {
            // read the value of the attribute
            uint8_t attr = *(attrBase + 32 * attrY + attrX);
            uint8_t inkColor = attr & 0b00000111;
            uint8_t paperColor = (attr & 0b00111000) >> 3;
            // check for changes in the attribute
            if ((attr & 0b10000000) != 0 && flashTimer < 16)
            {
                // we are flashing we need to swap the ink and paper colors
                uint8_t temp = inkColor;
                inkColor = paperColor;
                paperColor = temp;
                // update the attribute with the new colors - this makes our dirty check work
                attr = (attr & 0b11000000) | (inkColor & 0b00000111) | ((paperColor << 3) & 0b00111000);
            }
            if ((attr & 0b01000000) != 0)
            {
                inkColor = inkColor + 8;
                paperColor = paperColor + 8;
            }
            uint16_t tftInkColor = specpal565[inkColor];
            tftInkColor = (tftInkColor >> 8) | (tftInkColor << 8);

            uint16_t tftPaperColor = specpal565[paperColor];
            tftPaperColor = (tftPaperColor >> 8) | (tftPaperColor << 8);

            for (int y = 0; y < 8; y++)
            {
                // read the value of the pixels
                int screenY = attrY * 8 + y;
                int col = ((attrX * 8) & 0b11111000) >> 3;
                int scan = (screenY & 0b11000000) + ((screenY & 0b111) << 3) + ((screenY & 0b111000) >> 3);
                uint8_t row = *(pixelBase + 32 * scan + col);
                uint16_t *pixelAddress = pixelBuffer + WIDTH * screenY + attrX * 8 + borderWidth + borderHeight * WIDTH;
                for (int x = 0; x < 8; x++)
                {
                    if (row & 128)
                    {
                        *pixelAddress = tftInkColor;
                    }
                    else
                    {
                        *pixelAddress = tftPaperColor;
                    }
                    pixelAddress++;
                    row = row << 1;
                }
            }
        }
    }
    flashTimer++;
    if (flashTimer == 32)
    {
        flashTimer = 0;
    }
}

void main_loop()
{
    handleEvents(isRunning);
    count++;
    // fill out the framebuffer
    fillFrameBuffer(frameBuffer, machine->mem.currentScreen->data, machine->hwopt.BorderColor);
    updateAndRender(renderer, texture, frameBuffer);
    #ifndef __EMSCRIPTEN__
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    #endif
}

// Main function
int main()
{
    // start the emulator
    machine = new ZXSpectrum();
    machine->reset();
    machine->init_spectrum(SPECMDL_128K);
    machine->reset_spectrum(machine->z80Regs);

    if (!initSDL(window, renderer))
    {
        return -1;
    }

    texture = createTexture(renderer);
    if (!texture)
    {
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    // run the spectrum for 4 second so that it boots up
    int start = SDL_GetTicks();

    for(int i = 0; i < 200; i++) {
        machine->runForFrame(nullptr, nullptr);
    }
    // press the enter key to trigger tape loading
    machine->updateKey(SPECKEY_ENTER, 1);
    for(int i = 0; i < 10; i++) {
        machine->runForFrame(nullptr, nullptr);
    }
    machine->updateKey(SPECKEY_ENTER, 0);
    for(int i = 0; i < 10; i++) {
        machine->runForFrame(nullptr, nullptr);
    }
    int end = SDL_GetTicks();
    std::cout << "Time to boot spectrum: " << end - start << "ms" << std::endl;
    // load a tap file
    #if __EMSCRIPTEN__
    std::string filename = "filesystem/manic.z80";
    #else
    std::string filename = OpenFileDialog();
    #endif
    isLoading = true;
    loadGame(filename, machine);
    isLoading = false;
    audioOutput = new SDLAudioOutput(machine);
    audioOutput->start(15600);
    isRunning = true;
    #ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(main_loop, 50, 1);
    #else
    while (isRunning)
    {
        main_loop();
    }

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    #endif
    return 0;
}

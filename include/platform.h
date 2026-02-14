#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdbool.h>

#include "SDL.h"
#include "chip8.h"

typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_AudioSpec want;
    SDL_AudioSpec have;
    SDL_AudioDeviceID device;
} sdl_object;

bool init_sdl(sdl_object *sdl);
void cleanup(const sdl_object *sdl);
void clear_screen(const sdl_object *sdl);
void update_screen(const sdl_object *sdl, const chip8_object *chip8);
void update_timers(const sdl_object *sdl, chip8_object *chip8);
void handle_input(chip8_object *chip8);

#endif

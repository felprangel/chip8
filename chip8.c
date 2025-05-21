#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "SDL.h"

typedef struct {
    SDL_Window *window;
} sdl_object;

typedef struct {
    uint32_t window_width;
    uint32_t window_height;
} config_object;


bool init_sdl(sdl_object *sdl, const config_object config)
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) != 0) {
        SDL_Log("Could not initialize SDL subsystems! %s\n", SDL_GetError());
        return false;
    }

    u_int8_t no_flags = 0;
    sdl->window = SDL_CreateWindow(
        "Chip8 Emulator",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        config.window_width,
        config.window_height,
        no_flags
    );

    if (!sdl->window) {
        SDL_Log("Could not create SDL window %s\n", SDL_GetError());
        return false;
    }

    return true;
}

void cleanup(sdl_object *sdl)
{
    SDL_DestroyWindow(sdl->window);
    SDL_Quit();
}

int main(int argc, char *argv[])
{
    (void) argc;
    (void) argv;

    sdl_object sdl = {0};
    config_object config = {0};

    bool sdl_initialized = init_sdl(&sdl, config);

    if (!sdl_initialized) {
        exit(EXIT_FAILURE);
    }

    cleanup(&sdl);

    exit(EXIT_SUCCESS);
}

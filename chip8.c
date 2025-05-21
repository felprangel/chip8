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

    return true;
}

void cleanup(void)
{
    SDL_Quit();
}

int main(int argc, char *argv[])
{
    (void) argc;
    (void) argv;

    bool sdl_initialized = init_sdl();

    if (!sdl_initialized) {
        exit(EXIT_FAILURE);
    }

    cleanup();

    exit(EXIT_SUCCESS);
}

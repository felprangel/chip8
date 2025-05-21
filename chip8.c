#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "SDL.h"

#define WINDOW_HEIGHT 32
#define WINDOW_WIDTH 64

typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
} sdl_object;

bool init_sdl(sdl_object *sdl)
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
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        no_flags
    );

    if (!sdl->window) {
        SDL_Log("Could not create SDL window %s\n", SDL_GetError());
        return false;
    }

    int8_t sdl_driver_index = -1;
    sdl->renderer = SDL_CreateRenderer(sdl->window, sdl_driver_index, SDL_RENDERER_ACCELERATED);

    if (!sdl->renderer) {
        SDL_Log("Could not create SDL renderer %s\n", SDL_GetError());
        return false;
    }

    return true;
}

void cleanup(sdl_object *sdl)
{
    SDL_DestroyRenderer(sdl->renderer);
    SDL_DestroyWindow(sdl->window);
    SDL_Quit();
}

void clear_screen(sdl_object *sdl)
{
    SDL_SetRenderDrawColor(sdl->renderer, 255, 255, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(sdl->renderer);
}

int main(void)
{
    sdl_object sdl = {0};

    bool sdl_initialized = init_sdl(&sdl);

    if (!sdl_initialized) {
        exit(EXIT_FAILURE);
    }

    clear_screen(&sdl);

    while (true) {
        update_screen();
    }

    cleanup(&sdl);

    exit(EXIT_SUCCESS);
}

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "SDL.h"

#define WINDOW_HEIGHT 32
#define WINDOW_WIDTH 64
#define WINDOW_SCALE_FACTOR 20

typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
} sdl_object;

typedef enum {
    QUIT,
    RUNNING,
    PAUSED
} emulator_state;

typedef struct {
    emulator_state state;
} chip8_object;

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
        WINDOW_WIDTH * WINDOW_SCALE_FACTOR,
        WINDOW_HEIGHT * WINDOW_SCALE_FACTOR,
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

void cleanup(const sdl_object *sdl)
{
    SDL_DestroyRenderer(sdl->renderer);
    SDL_DestroyWindow(sdl->window);
    SDL_Quit();
}

void clear_screen(const sdl_object *sdl)
{
    SDL_SetRenderDrawColor(sdl->renderer, 255, 255, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(sdl->renderer);
}

void update_screen(const sdl_object *sdl)
{
    SDL_RenderPresent(sdl->renderer);
}

void handle_input(chip8_object *chip8)
{
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_QUIT:
            chip8->state = QUIT;
            break;

        case SDL_KEYDOWN:
            break;
        }
    }
}

bool init_chip8(chip8_object *chip8)
{
    chip8->state = RUNNING;
    return true;
}

int main(void)
{
    sdl_object sdl = {0};
    bool sdl_initialized = init_sdl(&sdl);

    if (!sdl_initialized) {
        exit(EXIT_FAILURE);
    }

    chip8_object chip8 = {0};
    bool chip8_initialized = init_chip8(&chip8);

    if (!chip8_initialized) {
        exit(EXIT_FAILURE);
    }

    clear_screen(&sdl);

    uint8_t sixty_hertz_in_ms = 16;

    while (chip8.state != QUIT) {
        handle_input(&chip8);
        SDL_Delay(sixty_hertz_in_ms);
        update_screen(&sdl);
    }

    cleanup(&sdl);

    exit(EXIT_SUCCESS);
}

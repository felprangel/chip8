#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "SDL.h"

#define WINDOW_HEIGHT 32
#define WINDOW_WIDTH 64
#define WINDOW_SCALE_FACTOR 20
#define WINDOW_HERTZ 60
#define WINDOW_UPDATE_MS 1000 / WINDOW_HERTZ

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
    uint8_t ram[4096];
    bool display[WINDOW_HEIGHT * WINDOW_WIDTH];
    uint16_t stack[12];
    uint8_t V[16];
    uint16_t I;
    uint16_t program_counter;
    uint8_t delay_timer;
    uint8_t sound_timer;
    bool keypad[16];
    char *rom_name;
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
    SDL_SetRenderDrawColor(sdl->renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
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
            switch (event.key.keysym.sym) {
                case SDLK_ESCAPE:
                    chip8->state = QUIT;
                    break;
                case SDLK_SPACE:
                    if (chip8->state == RUNNING) {
                        chip8->state = PAUSED;
                        puts("======== PAUSED ========");
                        break;
                    }

                    chip8->state = RUNNING;
                    break;
            }
            break;
        }
    }
}

bool init_chip8(chip8_object *chip8, char rom_name[])
{
    const uint32_t entrypoint = 0x200;
    const uint8_t font[] = {
        0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
        0x20, 0x60, 0x20, 0x20, 0x70, // 1
        0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
        0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
        0x90, 0x90, 0xF0, 0x10, 0x10, // 4
        0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
        0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
        0xF0, 0x10, 0x20, 0x40, 0x40, // 7
        0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
        0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
        0xF0, 0x90, 0xF0, 0x90, 0x90, // A
        0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
        0xF0, 0x80, 0x80, 0x80, 0xF0, // C
        0xE0, 0x90, 0x90, 0x90, 0xE0, // D
        0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
        0xF0, 0x80, 0xF0, 0x80, 0x80  // F
    };

    memcpy(&chip8->ram[0], font, sizeof(font));

    FILE *rom = fopen(rom_name, "rb");
    if (!rom) {
        SDL_Log("Rom file %s is invalid or does not exist\n", rom_name);
        return false;
    }
    fseek(rom, SEEK_SET, SEEK_END);
    const size_t rom_size = ftell(rom);
    const size_t max_size = sizeof chip8->ram - entrypoint;
    rewind(rom);

    if (rom_size > max_size) {
        SDL_Log("Rom file %s is too big!\n", rom_name);
        return false;
    }

    bool read_success = fread(&chip8->ram[entrypoint], rom_size, 1, rom);

    if (!read_success) {
        SDL_Log("Could not read rom file\n");
        return false;
    }

    fclose(rom);

    chip8->state = RUNNING;
    chip8->program_counter = entrypoint;
    chip8->rom_name = rom_name;
    return true;
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        printf("Rom name parameter needed\n");
        exit(EXIT_FAILURE);
    }

    sdl_object sdl = {0};
    bool sdl_initialized = init_sdl(&sdl);

    if (!sdl_initialized) {
        exit(EXIT_FAILURE);
    }

    chip8_object chip8 = {0};

    char *rom_name = argv[1];
    bool chip8_initialized = init_chip8(&chip8, rom_name);

    if (!chip8_initialized) {
        exit(EXIT_FAILURE);
    }

    clear_screen(&sdl);

    while (chip8.state != QUIT) {
        handle_input(&chip8);

        if (chip8.state == PAUSED) continue;

        SDL_Delay(WINDOW_UPDATE_MS);
        update_screen(&sdl);
    }

    cleanup(&sdl);

    exit(EXIT_SUCCESS);
}

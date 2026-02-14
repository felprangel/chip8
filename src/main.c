#include <stdio.h>
#include <stdlib.h>

#include "chip8.h"
#include "platform.h"

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

    const char *rom_name = argv[1];
    bool chip8_initialized = init_chip8(&chip8, rom_name);

    if (!chip8_initialized) {
        cleanup(&sdl);
        exit(EXIT_FAILURE);
    }

    clear_screen(&sdl);

    while (chip8.state != QUIT) {
        handle_input(&chip8);

        if (chip8.state == PAUSED) {
            continue;
        }

        const uint64_t start_frame = SDL_GetPerformanceCounter();

        for (uint32_t index = 0; index < INSTRUCTIONS_PER_SECOND / WINDOW_HERTZ; index++) {
            emulate_instruction(&chip8);
        }

        const uint64_t end_frame = SDL_GetPerformanceCounter();
        const double time_elapsed = (double)((end_frame - start_frame) * 1000) / SDL_GetPerformanceFrequency();

        SDL_Delay(WINDOW_UPDATE_MS > time_elapsed ? WINDOW_UPDATE_MS - time_elapsed : 0);
        update_screen(&sdl, &chip8);
        update_timers(&sdl, &chip8);
    }

    cleanup(&sdl);

    exit(EXIT_SUCCESS);
}

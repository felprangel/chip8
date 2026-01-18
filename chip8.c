#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#include "SDL.h"

#define WINDOW_HEIGHT 32
#define WINDOW_WIDTH 64
#define WINDOW_SCALE_FACTOR 20
#define WINDOW_HERTZ 60
#define WINDOW_UPDATE_MS 1000 / WINDOW_HERTZ
#define PIXEL_OUTLINE false

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
    uint16_t opcode;
    uint16_t NNN; // 12 bit address/constant
    uint8_t NN;   // 8 bit constant
    uint8_t N;    // 4 bit constant
    uint8_t X;    // 4 bit register identifier
    uint8_t Y;    // 4 bit register indentifier
} instruction_object;

typedef struct {
    emulator_state state;
    uint8_t ram[4096];
    bool display[WINDOW_HEIGHT * WINDOW_WIDTH];
    uint16_t stack[12];
    uint16_t *stack_pointer;
    uint8_t V[16];
    uint16_t I;
    uint16_t program_counter;
    uint8_t delay_timer;
    uint8_t sound_timer;
    bool keypad[16];
    const char *rom_name;
    instruction_object instruction;
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

void update_screen(const sdl_object sdl, const chip8_object chip8)
{
    SDL_Rect rectangle = {.x = 0, .y = 0, .w = WINDOW_SCALE_FACTOR, .h = WINDOW_SCALE_FACTOR};

    for (uint32_t index = 0; index < sizeof chip8.display; index++) {
        rectangle.x = (index % WINDOW_WIDTH) * WINDOW_SCALE_FACTOR;
        rectangle.y = (index / WINDOW_WIDTH) * WINDOW_SCALE_FACTOR;

        if (chip8.display[index]) {
            SDL_SetRenderDrawColor(sdl.renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
            SDL_RenderFillRect(sdl.renderer, &rectangle);

            if (PIXEL_OUTLINE) {
                SDL_SetRenderDrawColor(sdl.renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
                SDL_RenderDrawRect(sdl.renderer, &rectangle);
            }
        } else {
            SDL_SetRenderDrawColor(sdl.renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
            SDL_RenderFillRect(sdl.renderer, &rectangle);
        }
    }

    SDL_RenderPresent(sdl.renderer);
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

bool init_chip8(chip8_object *chip8, const char rom_name[])
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
    chip8->stack_pointer = &chip8->stack[0];
    return true;
}

void emulate_instruction(chip8_object *chip8)
{
    chip8->instruction.opcode = (chip8->ram[chip8->program_counter] << 8) | chip8->ram[chip8->program_counter + 1];
    chip8->program_counter += 2;

    chip8->instruction.NNN = chip8->instruction.opcode & 0x0FFF;
    chip8->instruction.NN = chip8->instruction.opcode & 0x0FF;
    chip8->instruction.N = chip8->instruction.opcode & 0x0F;
    chip8->instruction.X = (chip8->instruction.opcode >> 8) & 0x0F;
    chip8->instruction.Y = (chip8->instruction.opcode >> 4) & 0x0F;

    srand(time(NULL));

    switch ((chip8->instruction.opcode >> 12) & 0x0F)
    {
        case 0x00:
            if (chip8->instruction.NN == 0xE0) {
                // 0x00E0: Clear the screen
                memset(chip8->display, false, sizeof chip8->display);
            } else if (chip8->instruction.NN == 0xEE) {
                // 0x00EE: Return from subroutine
                chip8->program_counter = *--chip8->stack_pointer;
            } else {
                // Uninplemented/invalid opcode, may be 0xNNN for calling machine code routine for RCA1802
            }
            break;

        case 0x01:
            // 0x01NNN: Jump to address NNN
            chip8->program_counter = chip8->instruction.NNN;
            break;

        case 0x02:
            // 0x02NNN: Call subroutine at NNN
            *chip8->stack_pointer++ = chip8->program_counter;
            chip8->program_counter = chip8->instruction.NNN;
            break;

        case 0x03:
            // 0x03XNN: Check if VX == NN, if so, skip the next instruction
            if (chip8->V[chip8->instruction.X] == chip8->instruction.NN) {
                chip8->program_counter += 2; // skip next opcode/instruction
            }
            break;

        case 0x04:
            // 0x04NN: Check if VX != NN, if so, skip the next instruction
            if (chip8->V[chip8->instruction.X] != chip8->instruction.NN) {
                chip8->program_counter += 2; // skip next opcode/instruction
            }
            break;

        case 0x05:
            if (chip8->instruction.N != 0) break; // Invalid opcode
            // 0x05XY0: Check if VX == VY, if so, skip the next isntruction
            if (chip8->V[chip8->instruction.X] == chip8->V[chip8->instruction.Y]) {
                chip8->program_counter += 2; // skip next opcode/instruction
            }
            break;

        case 0x0A:
            // 0xANNN: Set index register I to NNN
            chip8->I = chip8->instruction.NNN;
            break;

        case 0x06:
            // 0x06XNN: Set register VX to NN
            chip8->V[chip8->instruction.X] = chip8->instruction.NN;
            break;

        case 0x07:
            // 0x07XNN: Set register VX += NN
            chip8->V[chip8->instruction.X] += chip8->instruction.NN;
            break;

        case 0x08:
            switch (chip8->instruction.N)
            {
                case 0:
                    // 0x08XY0: Set register VX = VY
                    chip8->V[chip8->instruction.X] = chip8->V[chip8->instruction.Y];
                    break;

                case 1:
                    // 0x08XY1: Set register VX |= VY
                    chip8->V[chip8->instruction.X] |= chip8->V[chip8->instruction.Y];
                    break;

                case 2:
                    // 0x08XY2: Set register VX &= VY
                    chip8->V[chip8->instruction.X] &= chip8->V[chip8->instruction.Y];
                    break;

                case 3:
                    // 0x08XY3: Set register VX ^= VY
                    chip8->V[chip8->instruction.X] ^= chip8->V[chip8->instruction.Y];
                    break;

                case 4:
                    // 0x08XY4: Set register VX += VY, set VF to 1 if carry
                    if ((uint16_t) (chip8->V[chip8->instruction.X] + chip8->V[chip8->instruction.Y]) > 255) {
                        chip8->V[0xF] = 1;
                    }

                    chip8->V[chip8->instruction.X] += chip8->V[chip8->instruction.Y];
                    break;

                case 5:
                    // 0x08XY5: Set register VX -= VY, set VF to 1 if there is not a borrow (result is positive)
                    if (chip8->V[chip8->instruction.Y] <= chip8->V[chip8->instruction.X]) {
                        chip8->V[0xF] = 1;
                    }

                    chip8->V[chip8->instruction.X] -= chip8->V[chip8->instruction.Y];
                    break;

                case 6:
                    // 0x08XY6: Set register VX >>=1, store shifted off bit in VF
                    chip8->V[0xF] = chip8->V[chip8->instruction.X] & 1;
                    chip8->V[chip8->instruction.X] >>= 1;
                    break;

                case 7:
                    // 0x08XY7: Set register VX = VY - VX, set VF to 1 if there is not a borrow (result is positive)
                    if (chip8->V[chip8->instruction.X] <= chip8->V[chip8->instruction.Y]) {
                        chip8->V[0xF] = 1;
                    }

                    chip8->V[chip8->instruction.X] = chip8->V[chip8->instruction.Y] - chip8->V[chip8->instruction.X];
                    break;

                case 0xE:
                    // 0x08XYE: Set register VX <<=1, store shifted off bit in VF
                    chip8->V[0xF] = (chip8->V[chip8->instruction.X] & 0x80) >> 7;
                    chip8->V[chip8->instruction.X] <<= 1;
                    break;

                default:
                    // Invalid opcode/instruction
                    break;
            }
            break;

        case 0x09:
            // 0x09XY0: Check if VX != VY; Skip next instruction if true
            if (chip8->V[chip8->instruction.X] != chip8->V[chip8->instruction.Y]) {
                chip8->program_counter += 2;
            }
            break;

        case 0x0B:
            // 0x0BNNN: Jump to V0 + NNN
            chip8->program_counter = chip8->V[0] + chip8->instruction.NNN;
            break;

        case 0x0C:
            // 0x0CXNN: Set register VX = rand() % 256 & NN (bitwise AND)
            chip8->V[chip8->instruction.X] = (rand() % 256) & chip8->instruction.NN;
            break;

        case 0x0D:
            // 0x0DXYN: Draw N-height sprite at coords X, Y; Read from memory location I;
            // Screen pixels are XOR'd with sprite bits,
            // VF (Carry flag) is set if any screen pixels are set off;

            uint8_t X_coord = chip8->V[chip8->instruction.X] % WINDOW_WIDTH;
            uint8_t Y_coord = chip8->V[chip8->instruction.Y] % WINDOW_HEIGHT;
            const uint8_t original_X = X_coord;

            chip8->V[0xF] = 0; // Initialize carry flag to 0

            for (uint8_t index = 0; index < chip8->instruction.N; index++) {
                const uint8_t sprite_data = chip8->ram[chip8->I + index];
                X_coord = original_X;

                for (int8_t j = 7; j >= 0; j--) {
                    bool *pixel = &chip8->display[Y_coord * WINDOW_WIDTH + X_coord];
                    const bool sprite_bit = (sprite_data & (1 << j));

                    if (sprite_bit && *pixel) {
                        chip8->V[0xF] = 1;
                    }

                    *pixel ^= sprite_bit;

                    if (++X_coord >= WINDOW_WIDTH) {
                        break;
                    }
                }

                if (++Y_coord >= WINDOW_HEIGHT) {
                    break;
                }
            }

            break;
        default:
            break;
    }
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

    const char *rom_name = argv[1];
    bool chip8_initialized = init_chip8(&chip8, rom_name);

    if (!chip8_initialized) {
        exit(EXIT_FAILURE);
    }

    clear_screen(&sdl);

    while (chip8.state != QUIT) {
        handle_input(&chip8);

        if (chip8.state == PAUSED) continue;

        emulate_instruction(&chip8);

        SDL_Delay(WINDOW_UPDATE_MS);
        update_screen(sdl, chip8);
    }

    cleanup(&sdl);

    exit(EXIT_SUCCESS);
}

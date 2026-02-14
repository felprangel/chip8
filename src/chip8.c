#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "SDL.h"
#include "chip8.h"

bool init_chip8(chip8_object *chip8, const char rom_name[])
{
    const uint32_t entrypoint = 0x200;
    const uint8_t font[] = {
        0xF0, 0x90, 0x90, 0x90, 0xF0,
        0x20, 0x60, 0x20, 0x20, 0x70,
        0xF0, 0x10, 0xF0, 0x80, 0xF0,
        0xF0, 0x10, 0xF0, 0x10, 0xF0,
        0x90, 0x90, 0xF0, 0x10, 0x10,
        0xF0, 0x80, 0xF0, 0x10, 0xF0,
        0xF0, 0x80, 0xF0, 0x90, 0xF0,
        0xF0, 0x10, 0x20, 0x40, 0x40,
        0xF0, 0x90, 0xF0, 0x90, 0xF0,
        0xF0, 0x90, 0xF0, 0x10, 0xF0,
        0xF0, 0x90, 0xF0, 0x90, 0x90,
        0xE0, 0x90, 0xE0, 0x90, 0xE0,
        0xF0, 0x80, 0x80, 0x80, 0xF0,
        0xE0, 0x90, 0x90, 0x90, 0xE0,
        0xF0, 0x80, 0xF0, 0x80, 0xF0,
        0xF0, 0x80, 0xF0, 0x80, 0x80
    };

    memcpy(&chip8->ram[0], font, sizeof(font));

    FILE *rom = fopen(rom_name, "rb");
    if (!rom) {
        SDL_Log("Rom file %s is invalid or does not exist\n", rom_name);
        return false;
    }

    fseek(rom, 0L, SEEK_END);
    const size_t rom_size = ftell(rom);
    const size_t max_size = sizeof chip8->ram - entrypoint;
    rewind(rom);

    if (rom_size > max_size) {
        SDL_Log("Rom file %s is too big!\n", rom_name);
        fclose(rom);
        return false;
    }

    bool read_success = fread(&chip8->ram[entrypoint], rom_size, 1, rom);

    if (!read_success) {
        SDL_Log("Could not read rom file\n");
        fclose(rom);
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

    bool positive_result = false;

    switch ((chip8->instruction.opcode >> 12) & 0x0F)
    {
        case 0x00:
            if (chip8->instruction.NN == 0xE0) {
                memset(chip8->display, false, sizeof chip8->display);
                break;
            }

            if (chip8->instruction.NN == 0xEE) {
                chip8->program_counter = *--chip8->stack_pointer;
                break;
            }
            break;

        case 0x01:
            chip8->program_counter = chip8->instruction.NNN;
            break;

        case 0x02:
            *chip8->stack_pointer++ = chip8->program_counter;
            chip8->program_counter = chip8->instruction.NNN;
            break;

        case 0x03:
            if (chip8->V[chip8->instruction.X] == chip8->instruction.NN) {
                chip8->program_counter += 2;
            }
            break;

        case 0x04:
            if (chip8->V[chip8->instruction.X] != chip8->instruction.NN) {
                chip8->program_counter += 2;
            }
            break;

        case 0x05:
            if (chip8->instruction.N != 0) {
                break;
            }
            if (chip8->V[chip8->instruction.X] == chip8->V[chip8->instruction.Y]) {
                chip8->program_counter += 2;
            }
            break;

        case 0x06:
            chip8->V[chip8->instruction.X] = chip8->instruction.NN;
            break;

        case 0x07:
            chip8->V[chip8->instruction.X] += chip8->instruction.NN;
            break;

        case 0x08:
            switch (chip8->instruction.N)
            {
                case 0:
                    chip8->V[chip8->instruction.X] = chip8->V[chip8->instruction.Y];
                    break;

                case 1:
                    chip8->V[chip8->instruction.X] |= chip8->V[chip8->instruction.Y];
                    chip8->V[0xF] = 0;
                    break;

                case 2:
                    chip8->V[chip8->instruction.X] &= chip8->V[chip8->instruction.Y];
                    chip8->V[0xF] = 0;
                    break;

                case 3:
                    chip8->V[chip8->instruction.X] ^= chip8->V[chip8->instruction.Y];
                    chip8->V[0xF] = 0;
                    break;

                case 4: {
                    const bool carry =
                        ((uint16_t)(chip8->V[chip8->instruction.X] + chip8->V[chip8->instruction.Y]) > 255);
                    chip8->V[chip8->instruction.X] += chip8->V[chip8->instruction.Y];
                    chip8->V[0xF] = carry;
                    break;
                }

                case 5:
                    positive_result = (chip8->V[chip8->instruction.Y] <= chip8->V[chip8->instruction.X]);
                    chip8->V[chip8->instruction.X] -= chip8->V[chip8->instruction.Y];
                    chip8->V[0xF] = positive_result;
                    break;

                case 6:
                    positive_result = chip8->V[chip8->instruction.Y] & 1;
                    chip8->V[chip8->instruction.X] = chip8->V[chip8->instruction.Y] >> 1;
                    chip8->V[0xF] = positive_result;
                    break;

                case 7:
                    positive_result = (chip8->V[chip8->instruction.X] <= chip8->V[chip8->instruction.Y]);
                    chip8->V[chip8->instruction.X] = chip8->V[chip8->instruction.Y] - chip8->V[chip8->instruction.X];
                    chip8->V[0xF] = positive_result;
                    break;

                case 0xE:
                    positive_result = (chip8->V[chip8->instruction.Y] & 0x80) >> 7;
                    chip8->V[chip8->instruction.X] = chip8->V[chip8->instruction.Y] << 1;
                    chip8->V[0xF] = positive_result;
                    break;

                default:
                    break;
            }
            break;

        case 0x09:
            if (chip8->V[chip8->instruction.X] != chip8->V[chip8->instruction.Y]) {
                chip8->program_counter += 2;
            }
            break;

        case 0x0A:
            chip8->I = chip8->instruction.NNN;
            break;

        case 0x0B:
            chip8->program_counter = chip8->V[0] + chip8->instruction.NNN;
            break;

        case 0x0C:
            chip8->V[chip8->instruction.X] = (rand() % 256) & chip8->instruction.NN;
            break;

        case 0x0D: {
            uint8_t x_coord = chip8->V[chip8->instruction.X] % WINDOW_WIDTH;
            uint8_t y_coord = chip8->V[chip8->instruction.Y] % WINDOW_HEIGHT;
            const uint8_t original_x = x_coord;

            chip8->V[0xF] = 0;

            for (uint8_t index = 0; index < chip8->instruction.N; index++) {
                const uint8_t sprite_data = chip8->ram[chip8->I + index];
                x_coord = original_x;

                for (int8_t j = 7; j >= 0; j--) {
                    bool *pixel = &chip8->display[y_coord * WINDOW_WIDTH + x_coord];
                    const bool sprite_bit = (sprite_data & (1 << j));

                    if (sprite_bit && *pixel) {
                        chip8->V[0xF] = 1;
                    }

                    *pixel ^= sprite_bit;

                    if (++x_coord >= WINDOW_WIDTH) {
                        break;
                    }
                }

                if (++y_coord >= WINDOW_HEIGHT) {
                    break;
                }
            }

            break;
        }

        case 0x0E:
            if (chip8->instruction.NN == 0x9E) {
                uint8_t vx = chip8->V[chip8->instruction.X];
                if (chip8->keypad[vx]) {
                    chip8->program_counter += 2;
                }
                break;
            }
            if (chip8->instruction.NN == 0xA1) {
                uint8_t vx = chip8->V[chip8->instruction.X];
                if (!chip8->keypad[vx]) {
                    chip8->program_counter += 2;
                }
                break;
            }
            break;

        case 0x0F:
            switch (chip8->instruction.NN)
            {
                case 0x07:
                    chip8->V[chip8->instruction.X] = chip8->delay_timer;
                    break;

                case 0x0A: {
                    static bool any_key_pressed = false;
                    static int8_t key = -1;

                    for (uint8_t index = 0; index < sizeof chip8->keypad; index++) {
                        if (chip8->keypad[index]) {
                            key = (int8_t)index;
                            any_key_pressed = true;
                            break;
                        }
                    }

                    if (!any_key_pressed) {
                        chip8->program_counter -= 2;
                        break;
                    }

                    if (chip8->keypad[(uint8_t)key]) {
                        chip8->program_counter -= 2;
                        break;
                    }

                    chip8->V[chip8->instruction.X] = (uint8_t)key;
                    key = -1;
                    any_key_pressed = false;

                    break;
                }

                case 0x15:
                    chip8->delay_timer = chip8->V[chip8->instruction.X];
                    break;

                case 0x18:
                    chip8->sound_timer = chip8->V[chip8->instruction.X];
                    break;

                case 0x1E:
                    chip8->I += chip8->V[chip8->instruction.X];
                    break;

                case 0x29:
                    chip8->I = chip8->V[chip8->instruction.X] * 5;
                    break;

                case 0x33: {
                    uint8_t bcd = chip8->V[chip8->instruction.X];

                    chip8->ram[chip8->I + 2] = bcd % 10;
                    bcd /= 10;
                    chip8->ram[chip8->I + 1] = bcd % 10;
                    bcd /= 10;
                    chip8->ram[chip8->I] = bcd;
                    break;
                }

                case 0x55:
                    for (uint8_t index = 0; index <= chip8->instruction.X; index++) {
                        chip8->ram[chip8->I++] = chip8->V[index];
                    }
                    break;

                case 0x65:
                    for (uint8_t index = 0; index <= chip8->instruction.X; index++) {
                        chip8->V[index] = chip8->ram[chip8->I++];
                    }
                    break;

                default:
                    break;
            }
            break;

        default:
            break;
    }
}

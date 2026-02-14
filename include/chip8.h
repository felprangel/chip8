#ifndef CHIP8_H
#define CHIP8_H

#include <stdbool.h>
#include <stdint.h>

#define WINDOW_HEIGHT 32
#define WINDOW_WIDTH 64
#define WINDOW_SCALE_FACTOR 20
#define WINDOW_HERTZ 60
#define WINDOW_UPDATE_MS 1000 / WINDOW_HERTZ
#define PIXEL_OUTLINE false
#define INSTRUCTIONS_PER_SECOND 700
#define SOUND_WAVE_FREQUENCY 440
#define AUDIO_SAMPLE_RATE 44100
#define VOLUME 3000

typedef enum {
    QUIT,
    RUNNING,
    PAUSED
} emulator_state;

typedef struct {
    uint16_t opcode;
    uint16_t NNN;
    uint8_t NN;
    uint8_t N;
    uint8_t X;
    uint8_t Y;
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

bool init_chip8(chip8_object *chip8, const char rom_name[]);
void emulate_instruction(chip8_object *chip8);

#endif

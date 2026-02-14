#include <stdio.h>

#include "platform.h"

static void audio_callback(void *userdata, uint8_t *stream, int length)
{
    (void)userdata;

    int16_t *audio_data = (int16_t *)stream;
    static uint32_t running_sample_index = 0;

    const int32_t square_wave_period = AUDIO_SAMPLE_RATE / SOUND_WAVE_FREQUENCY;
    const int32_t half_square_wave_period = square_wave_period / 2;

    for (int index = 0; index < length / 2; index++) {
        audio_data[index] = ((running_sample_index++ / half_square_wave_period) % 2) ? VOLUME : -VOLUME;
    }
}

bool init_sdl(sdl_object *sdl)
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) != 0) {
        SDL_Log("Could not initialize SDL subsystems! %s\n", SDL_GetError());
        return false;
    }

    uint8_t no_flags = 0;
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

    sdl->want = (SDL_AudioSpec){
        .freq = 44100,
        .format = AUDIO_S16LSB,
        .channels = 1,
        .samples = 512,
        .callback = audio_callback,
    };

    sdl->device = SDL_OpenAudioDevice(NULL, 0, &sdl->want, &sdl->have, 0);

    if (sdl->device == 0) {
        SDL_Log("Could not get Audio Device %s\n", SDL_GetError());
        return false;
    }

    if ((sdl->want.channels != sdl->have.channels) || (sdl->want.format != sdl->have.format)) {
        SDL_Log("Could not get desired Audio Spec\n");
        return false;
    }

    return true;
}

void cleanup(const sdl_object *sdl)
{
    SDL_DestroyRenderer(sdl->renderer);
    SDL_DestroyWindow(sdl->window);
    SDL_CloseAudioDevice(sdl->device);
    SDL_Quit();
}

void clear_screen(const sdl_object *sdl)
{
    SDL_SetRenderDrawColor(sdl->renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(sdl->renderer);
}

void update_screen(const sdl_object *sdl, const chip8_object *chip8)
{
    SDL_Rect rectangle = {.x = 0, .y = 0, .w = WINDOW_SCALE_FACTOR, .h = WINDOW_SCALE_FACTOR};

    for (uint32_t index = 0; index < sizeof chip8->display; index++) {
        rectangle.x = (index % WINDOW_WIDTH) * WINDOW_SCALE_FACTOR;
        rectangle.y = (index / WINDOW_WIDTH) * WINDOW_SCALE_FACTOR;

        if (chip8->display[index]) {
            SDL_SetRenderDrawColor(sdl->renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
            SDL_RenderFillRect(sdl->renderer, &rectangle);

            if (PIXEL_OUTLINE) {
                SDL_SetRenderDrawColor(sdl->renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
                SDL_RenderDrawRect(sdl->renderer, &rectangle);
            }

            continue;
        }

        SDL_SetRenderDrawColor(sdl->renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
        SDL_RenderFillRect(sdl->renderer, &rectangle);
    }

    SDL_RenderPresent(sdl->renderer);
}

void update_timers(const sdl_object *sdl, chip8_object *chip8)
{
    if (chip8->delay_timer > 0) {
        chip8->delay_timer--;
    }

    if (chip8->sound_timer > 0) {
        chip8->sound_timer--;
        SDL_PauseAudioDevice(sdl->device, 0);
        return;
    }

    SDL_PauseAudioDevice(sdl->device, 1);
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

                case SDLK_1:
                    chip8->keypad[0x1] = true;
                    break;

                case SDLK_2:
                    chip8->keypad[0x2] = true;
                    break;

                case SDLK_3:
                    chip8->keypad[0x3] = true;
                    break;

                case SDLK_4:
                    chip8->keypad[0xC] = true;
                    break;

                case SDLK_q:
                    chip8->keypad[0x4] = true;
                    break;

                case SDLK_w:
                    chip8->keypad[0x5] = true;
                    break;

                case SDLK_e:
                    chip8->keypad[0x6] = true;
                    break;

                case SDLK_r:
                    chip8->keypad[0xD] = true;
                    break;

                case SDLK_a:
                    chip8->keypad[0x7] = true;
                    break;

                case SDLK_s:
                    chip8->keypad[0x8] = true;
                    break;

                case SDLK_d:
                    chip8->keypad[0x9] = true;
                    break;

                case SDLK_f:
                    chip8->keypad[0xE] = true;
                    break;

                case SDLK_z:
                    chip8->keypad[0xA] = true;
                    break;

                case SDLK_x:
                    chip8->keypad[0x0] = true;
                    break;

                case SDLK_c:
                    chip8->keypad[0xB] = true;
                    break;

                case SDLK_v:
                    chip8->keypad[0xF] = true;
                    break;
            }
            break;

        case SDL_KEYUP:
            switch (event.key.keysym.sym)
            {
                case SDLK_1:
                    chip8->keypad[0x1] = false;
                    break;

                case SDLK_2:
                    chip8->keypad[0x2] = false;
                    break;

                case SDLK_3:
                    chip8->keypad[0x3] = false;
                    break;

                case SDLK_4:
                    chip8->keypad[0xC] = false;
                    break;

                case SDLK_q:
                    chip8->keypad[0x4] = false;
                    break;

                case SDLK_w:
                    chip8->keypad[0x5] = false;
                    break;

                case SDLK_e:
                    chip8->keypad[0x6] = false;
                    break;

                case SDLK_r:
                    chip8->keypad[0xD] = false;
                    break;

                case SDLK_a:
                    chip8->keypad[0x7] = false;
                    break;

                case SDLK_s:
                    chip8->keypad[0x8] = false;
                    break;

                case SDLK_d:
                    chip8->keypad[0x9] = false;
                    break;

                case SDLK_f:
                    chip8->keypad[0xE] = false;
                    break;

                case SDLK_z:
                    chip8->keypad[0xA] = false;
                    break;

                case SDLK_x:
                    chip8->keypad[0x0] = false;
                    break;

                case SDLK_c:
                    chip8->keypad[0xB] = false;
                    break;

                case SDLK_v:
                    chip8->keypad[0xF] = false;
                    break;
                default:
                    break;
            }
        }
    }
}

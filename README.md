# CHIP-8 Emulator (C + SDL2)

Emulador CHIP-8 escrito em C usando SDL2 para janela, render e audio.

## Estrutura do projeto

```text
.
├── include/
│   ├── chip8.h      # Estado do emulador, constantes e API do core
│   └── platform.h   # API SDL/plataforma (janela, input, audio, timers)
├── src/
│   ├── chip8.c      # Inicializacao da VM e execucao de instrucoes (opcodes)
│   ├── platform.c   # Implementacao SDL (render, teclado, audio)
│   └── main.c       # Loop principal e coordenacao entre core e plataforma
└── Makefile
```

## Dependencias

- GCC/Clang com suporte a C17
- SDL2
- `sdl2-config` no PATH

## Build

```bash
make
```

## Execucao

```bash
./chip8 caminho/para/rom.ch8
```

## Controles

- `ESC`: sair
- `SPACE`: pausar/continuar
- Teclado CHIP-8:

```text
1 2 3 4
Q W E R
A S D F
Z X C V
```

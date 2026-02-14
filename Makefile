CFLAGS=-std=c17 -Wall -Wextra -Werror -Iinclude
LDFLAGS=`sdl2-config --libs`
SDL_CFLAGS=`sdl2-config --cflags`
SRC=$(wildcard src/*.c)
OBJ=$(SRC:.c=.o)
BIN=chip8

all: $(BIN)

$(BIN): $(OBJ)
	gcc $(OBJ) -o $(BIN) $(LDFLAGS)

src/%.o: src/%.c
	gcc $(CFLAGS) $(SDL_CFLAGS) -c $< -o $@

clean:
	rm -f src/*.o $(BIN)

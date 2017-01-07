all: game_loop.c
	gcc -g -Wall -o chip8 game_loop.c

clean:
	$(RM) chip8
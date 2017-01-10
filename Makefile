all: game_loop.c
	gcc -g -Wall -o chip8 game_loop.c -L/System/Library/Frameworks -framework GLUT -framework OpenGL -Wno-deprecated-declarations

clean:
	$(RM) chip8
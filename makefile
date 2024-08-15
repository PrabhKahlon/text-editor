CFLAGS=-Wall -Wextra -std=c11 -pedantic -ggdb `pkg-config --cflags sdl2 SDL2_ttf`
LIBS=`pkg-config --libs sdl2 SDL2_ttf` -lm

text: main.c
	  $(CC) $(CFLAGS) -o text main.c vec.c glyph.c gap.c line.c $(LIBS)
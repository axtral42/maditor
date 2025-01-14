CFLAGS= -Wall -Wextra -std=c11 -pedantic -ggdb `pkg-config --cflags sdl2`
LIBS=`pkg-config --libs sdl2` -lm

maditor: main.c
	$(CC) $(CFLAGS) -o maditor main.c la.c editor.c $(LIBS)

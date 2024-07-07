CC = gcc
CFLAGS = -Isrc/include -Wall -Wextra -g
LDFLAGS = -Lsrc/lib -lmingw32 -Isrc/include/cglm/include -lSDL2main -lSDL2 -lSDL2_ttf -lglew32 -lglfw3 -mconsole -lopengl32 -lm

OBJS = gameloop.o rendering.o

bin/gameloop.exe: $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

gameloop.o: gameloop.c rendering.h
	$(CC) $(CFLAGS) -c gameloop.c

rendering.o: rendering.c rendering.h
	$(CC) $(CFLAGS) -c rendering.c

clean:
	rm -f $(OBJS) bin/gameloop.exe

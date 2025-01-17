# Existing Makefile
CC = gcc
CFLAGS = -Isrc/include -Wall -Wextra -g -std=c11
LDFLAGS = -Lsrc/lib -lmingw32 -Isrc/include/cglm/include -lSDL2main -lSDL2 -lSDL2_ttf -lglew32 -lglfw3 -mconsole -lopengl32 -lm -latomic
OBJS = gameloop.o rendering.o player.o enemy.o grid.o pathfinding.o entity.o asciiMap.o saveload.o structures.o input.o ui.o inventory.o item.o texture_coords.o

gameloop.exe: $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

gameloop.o: gameloop.c gameloop.h rendering.h player.h entity.h enemy.h grid.h pathfinding.h asciiMap.h input.h texture_coords.h
	$(CC) $(CFLAGS) -c gameloop.c

rendering.o: rendering.c rendering.h texture_coords.h
	$(CC) $(CFLAGS) -c rendering.c

asciiMap.o: asciiMap.c asciiMap.h grid.h
	$(CC) $(CFLAGS) -c asciiMap.c

texture_coords.o: texture_coords.c texture_coords.h
	$(CC) $(CFLAGS) -c texture_coords.c

clean:
	rm -f $(OBJS) bin/gameloop.exe

# Test build section
TEST_OBJS = test_enemy.o enemy.o entity.o grid.o pathfinding.o player.o

test: $(TEST_OBJS)
	$(CC) -o bin/test_enemy $^ $(LDFLAGS) -lm

test_enemy.o: test_enemy.c enemy.h entity.h grid.h pathfinding.h player.h imintrin.h
	$(CC) $(CFLAGS) -c test_enemy.c

clean_tests:
	rm -f $(TEST_OBJS) bin/test_enemy
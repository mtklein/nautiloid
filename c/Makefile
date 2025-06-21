CC=gcc
CFLAGS=`sdl2-config --cflags` -Wall -Wextra -g
LDFLAGS=`sdl2-config --libs`

all: main

main: main.o
$(CC) $(CFLAGS) -o main main.o $(LDFLAGS)

main.o: main.c
$(CC) $(CFLAGS) -c main.c

clean:
rm -f *.o main

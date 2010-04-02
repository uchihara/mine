CFLAGS = -Wall -g -O2
CFLAGS = -Wall -g
all: mine

mine: mine.o
	cc -o $@ $< -lncurses

clean:
	rm -f mine mine.o

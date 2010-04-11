CFLAGS = -Wall -g -O2
CFLAGS = -Wall -g
OBJS = mine.o screens.o fields.o signals.o

all: mine

mine: $(OBJS)
	$(CC) -o $@ $(OBJS) -lcurses

clean:
	rm -f mine $(OBJS)

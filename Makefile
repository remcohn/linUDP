#hellomake: linUDP.c misc.c ticker.c
#	gcc -std=c11 -o linUDP linUDP.c misc.c ticker.c -I. -lm -lpthread 

CC = gcc
CFLAGS = -g -std=c11 -I. -Wall -Wextra `pkg-config --cflags gtk+-3.0`
LDFLAGS = -lm -lpthread `pkg-config --libs gtk+-3.0`

OBJS = linUDP.o misc.o ticker.o gui.o buttons.o c1250.o

linUDP: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o linUDP

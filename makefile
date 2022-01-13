CC = gcc
CFLAGS = -std=c99 -Wall

virmem: main.o
	$(CC) main.o -o virmem

main.o: main.c
	$(CC) $(CFLAGS) -c main.c

clean:
	rm -f virmem main.o
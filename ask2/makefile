.PHONY: all clean

all: fork-example ask21_main tree-example ask22_main ask23_main ask24_main

CC = gcc
CFLAGS = -g -Wall -O2
SHELL= /bin/bash

proc-common.o: proc-common.c proc-common.h
	$(CC) $(CFLAGS)  -o proc-common.o -c proc-common.c

fork-example: fork-example.o proc-common.o
	$(CC) $(CFLAGS) $^ -o $@

fork-example.o: fork-example.c
	$(CC) $(CFLAGS) -c fork-example.c

ask24_main.o: ask24_main.c
	$(CC) $(CFLAGS) -c ask24_main.c

ask24_main:ask24_main.o proc-common.o tree.o
	$(CC) $(CFLAGS) $^ -o $@

ask23_main.o: ask23_main.c
	$(CC) $(CFLAGS) -c ask23_main.c

ask23_main: ask23_main.o proc-common.o tree.o
	$(CC) $(CFLAGS) $^ -o $@

ask22_main.o:ask22_main.c
	$(CC) $(CFLAGS) -c ask22_main.c

ask22_main:ask22_main.o proc-common.o tree.o
	$(CC) $(CFLAGS) $^ -o $@

ask21_main.o: ask21_main.c
	$(CC) $(CFLAGS) -c ask21_main.c

ask21_main: ask21_main.o proc-common.o
	$(CC) $(CFLAGS) $^ -o $@

tree.o: tree.h tree.c
	$(CC) $(CFLAGS)  -o tree.o -c tree.c

tree-example.o: tree-example.c
	$(CC) $(CFLAGS) -c tree-example.c

tree-example: tree-example.o tree.o
	$(CC) $(CFLAGS)  $^ -o $@

%.s: %.c
	$(CC) $(CFLAGS) -S -fverbose-asm $<

%.o: %.c
	$(CC) $(CFLAGS) -c $<

%.i: %.c
	gcc -Wall -E $< | indent -kr > $@

clean:
	rm -f *.o fork-example ask2-{fork}


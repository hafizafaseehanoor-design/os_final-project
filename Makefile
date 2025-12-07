CC=gcc
CFLAGS=-Wall -g

SRC=src/main.c src/parser.c src/jobs.c

all:
	$(CC) $(CFLAGS) $(SRC) -o myshell
